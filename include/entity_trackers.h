#pragma once

#include "entity_helpers_common.h"

// Trackers are subscription-driven: Home Assistant pushes the current value of
// every subscribed state/attribute right after the API handshake and streams
// changes afterwards, so no explicit fetches are needed. Note that
// api::APIServer::get_home_assistant_state() must NOT be used for periodic
// refreshes: each call permanently appends to the server's subscription
// vector (it is never pruned), and entries added after the handshake are never
// announced to Home Assistant — a heap leak that fetches nothing.
//
// Subscriptions go through APIServer's const char* overload. Entity IDs live in
// the favorite lists, which never change while the remote is awake (see
// favorites_store.h), and attribute names are string literals, so the server
// can keep the pointers as-is, and every callback below captures at most
// 8 bytes, which std::function stores inline. The CustomAPIDevice helpers would
// instead heap-copy the entity ID twice, the attribute once and the closure
// once for every subscription — several hundred allocations for a typical
// favorites setup — and the callback would then have to search for the index
// it could have captured.
//
// The API server announces one subscription per main-loop pass, so each extra
// attribute delays the initial sync after every wake. Only subscribe to
// attributes Home Assistant actually exposes and the remote actually reads.

using HaStateCallback = std::function<void(esphome::StringRef)>;

// attribute == nullptr subscribes to the entity state itself.
inline void ha_subscribe(const char *entity_id, const char *attribute, HaStateCallback &&callback) {
  esphome::api::global_api_server->subscribe_home_assistant_state(entity_id, attribute, std::move(callback));
}

// Stores the value. "unavailable" is kept, so the remote can say so; anything
// else missing becomes "unknown".
inline void ha_track_state(const char *entity_id, const char *attribute, std::string &slot) {
  std::string *target = &slot;
  ha_subscribe(entity_id, attribute, [target](esphome::StringRef state) { ha_assign_state_or_unknown(*target, state); });
}

// Stores the raw value.
inline void ha_track_text(const char *entity_id, const char *attribute, std::string &slot) {
  std::string *target = &slot;
  ha_subscribe(entity_id, attribute, [target](esphome::StringRef state) { ha_assign(*target, state); });
}

// Stores a JSON array attribute as a '|'-joined option list.
inline void ha_track_list(const char *entity_id, const char *attribute, std::string &slot) {
  std::string *target = &slot;
  ha_subscribe(entity_id, attribute, [target](esphome::StringRef state) { ha_store_joined_list(*target, state); });
}

// Stores a numeric value, NAN when missing or unparseable.
inline void ha_track_float(const char *entity_id, const char *attribute, float &slot) {
  float *target = &slot;
  ha_subscribe(entity_id, attribute, [target](esphome::StringRef state) { *target = ha_parse_float(state); });
}

// Stores an integer attribute such as supported_features. The slot keeps its
// default until Home Assistant sends a number.
inline void ha_track_int(const char *entity_id, const char *attribute, int &slot) {
  int *target = &slot;
  ha_subscribe(entity_id, attribute, [target](esphome::StringRef state) {
    if (ha_state_missing(state)) {
      return;
    }
    char buffer[16];
    size_t len = state.size() < sizeof(buffer) - 1 ? state.size() : sizeof(buffer) - 1;
    memcpy(buffer, state.c_str(), len);
    buffer[len] = '\0';
    char *end = nullptr;
    long value = strtol(buffer, &end, 10);
    if (end != buffer) {
      *target = static_cast<int>(value);
    }
  });
}

// Common base: one Slot of values for each favorite of the tracker's mode,
// and bounds-checked accessors. allocate() sizes the slots once, before any
// subscription: the subscriptions keep pointers into them, so they never move.
// Subclasses implement subscribe(idx), which registers every subscription for
// one entity, and give SUBSCRIPTIONS, the most that takes.
template <typename Slot>
class EntityTracker {
 public:
  using SlotType = Slot;

  explicit EntityTracker(RemoteMode mode) : mode_(mode) {}

  void allocate() {
    this->count_ = mode_entity_count(this->mode_);
    this->slots_.reset(new Slot[this->count_]);
  }

  RemoteMode tracked_mode() const { return this->mode_; }
  int count() const { return this->count_; }
  // "" for an index out of range.
  const char *entity_id(int idx) const { return mode_item_entity_cstr(this->mode_, idx); }

 protected:
  bool in_range_(int idx) const { return idx >= 0 && idx < this->count_; }

  const std::string &at_(int idx, std::string Slot::*field, const std::string &fallback = empty_string()) const {
    return this->in_range_(idx) ? this->slots_[idx].*field : fallback;
  }

  float at_(int idx, float Slot::*field) const { return this->in_range_(idx) ? this->slots_[idx].*field : NAN; }

  RemoteMode mode_;
  int count_ = 0;
  std::unique_ptr<Slot[]> slots_;
};

struct StateSlot {
  std::string state;
};

template <typename Slot = StateSlot>
class SingleStateTracker : public EntityTracker<Slot> {
 public:
  static constexpr int SUBSCRIPTIONS = 1;

  explicit SingleStateTracker(RemoteMode mode) : EntityTracker<Slot>(mode) {}

  void subscribe(int idx) { ha_track_state(this->entity_id(idx), nullptr, this->slots_[idx].state); }

  const std::string &state(int idx) const { return this->at_(idx, &Slot::state, unknown_string()); }
};

struct LightSlot {
  std::string state;
  std::string color_modes;
  std::string effect;
  std::string effect_list;
  float brightness = NAN;
  float color_temp_kelvin = NAN;
  float min_color_temp_kelvin = NAN;
  float max_color_temp_kelvin = NAN;
  float hue = NAN;
  float saturation = NAN;
};

class LightStatusTracker : public EntityTracker<LightSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 5 + (LIGHT_WARMTH ? 3 : 0) + (LIGHT_COLOR ? 1 : 0);

  LightStatusTracker() : EntityTracker(REMOTE_MODE_LIGHTS) {}

  // supported_color_modes is the only reliable sign of a light that can't dim:
  // brightness is missing from every light that is off, and from one that
  // hasn't synced yet. The colour temperature and its range are only sent by
  // lights that have one.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    LightSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_float(entity_id, "brightness", slot.brightness);
    ha_track_list(entity_id, "supported_color_modes", slot.color_modes);
    ha_track_text(entity_id, "effect", slot.effect);
    ha_track_list(entity_id, "effect_list", slot.effect_list);
    // Home Assistant passes a colour temperature to the light without
    // checking it against the light's range, so the remote needs the range.
    if (LIGHT_WARMTH) {
      ha_track_float(entity_id, "color_temp_kelvin", slot.color_temp_kelvin);
      ha_track_float(entity_id, "min_color_temp_kelvin", slot.min_color_temp_kelvin);
      ha_track_float(entity_id, "max_color_temp_kelvin", slot.max_color_temp_kelvin);
    }
    // "(30.0, 70.0)"; "None" while the light is off.
    if (LIGHT_COLOR) {
      float *hue = &slot.hue;
      float *saturation = &slot.saturation;
      ha_subscribe(entity_id, "hs_color", [hue, saturation](esphome::StringRef state) {
        if (!parse_hs_color(state.c_str(), state.size(), hue, saturation)) {
          *hue = NAN;
          *saturation = NAN;
        }
      });
    }
  }

  const std::string &state(int idx) const { return at_(idx, &LightSlot::state, unknown_string()); }
  float brightness(int idx) const { return at_(idx, &LightSlot::brightness); }
  bool has_brightness(int idx) const { return !std::isnan(this->brightness(idx)); }
  // -1 until the color modes arrive, 0 for an on/off-only light, 1 if it dims.
  int dimmable(int idx) const { return light_modes_dimmable(at_(idx, &LightSlot::color_modes)); }
  const std::string &effect(int idx) const { return at_(idx, &LightSlot::effect); }
  const std::string &effect_list(int idx) const { return at_(idx, &LightSlot::effect_list); }
  bool has_effect(int idx) const { return !this->effect_list(idx).empty(); }
  bool supports_color_temp(int idx) const {
    return LIGHT_WARMTH && light_modes_color_temp(at_(idx, &LightSlot::color_modes));
  }
  // NAN while the light is off, or in a colour mode.
  float color_temp_kelvin(int idx) const { return at_(idx, &LightSlot::color_temp_kelvin); }
  float min_color_temp_kelvin(int idx) const { return at_(idx, &LightSlot::min_color_temp_kelvin); }
  float max_color_temp_kelvin(int idx) const { return at_(idx, &LightSlot::max_color_temp_kelvin); }
  bool supports_color(int idx) const { return LIGHT_COLOR && light_modes_color(at_(idx, &LightSlot::color_modes)); }
  // NAN while the light is off.
  float hue(int idx) const { return at_(idx, &LightSlot::hue); }
  float saturation(int idx) const { return at_(idx, &LightSlot::saturation); }
};

using SwitchStatusTracker = SingleStateTracker<>;

struct FanSlot {
  std::string state;
  std::string preset_mode;
  std::string preset_modes;
  std::string oscillating;
  std::string direction;
  float percentage = NAN;
  float percentage_step = NAN;
};

class FanStatusTracker : public EntityTracker<FanSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 7;

  FanStatusTracker() : EntityTracker(REMOTE_MODE_FANS) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    FanSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_float(entity_id, "percentage", slot.percentage);
    ha_track_float(entity_id, "percentage_step", slot.percentage_step);
    ha_track_text(entity_id, "preset_mode", slot.preset_mode);
    ha_track_list(entity_id, "preset_modes", slot.preset_modes);
    ha_track_text(entity_id, "oscillating", slot.oscillating);
    ha_track_text(entity_id, "direction", slot.direction);
  }

  const std::string &state(int idx) const { return at_(idx, &FanSlot::state, unknown_string()); }
  float percentage(int idx) const { return at_(idx, &FanSlot::percentage); }
  bool has_percentage(int idx) const { return !std::isnan(this->percentage(idx)); }
  float percentage_step(int idx) const { return at_(idx, &FanSlot::percentage_step); }
  // Home Assistant reports percentage and percentage_step for every fan with
  // speeds, but many report the percentage as None while off: the step says
  // the fan has speeds before a percentage arrives.
  bool supports_speed(int idx) const { return this->has_percentage(idx) || !std::isnan(this->percentage_step(idx)); }
  const std::string &preset_mode(int idx) const { return at_(idx, &FanSlot::preset_mode); }
  const std::string &preset_modes(int idx) const { return at_(idx, &FanSlot::preset_modes); }
  const std::string &oscillating(int idx) const { return at_(idx, &FanSlot::oscillating); }
  const std::string &direction(int idx) const { return at_(idx, &FanSlot::direction); }
};

struct HumidifierSlot {
  std::string state;
  std::string mode;
  std::string available_modes;
  std::string action;
  float target_humidity = NAN;
  float current_humidity = NAN;
  float min_humidity = NAN;
  float max_humidity = NAN;
};

class HumidifierStatusTracker : public EntityTracker<HumidifierSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 8;

  HumidifierStatusTracker() : EntityTracker(REMOTE_MODE_HUMIDIFIERS) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    HumidifierSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_text(entity_id, "mode", slot.mode);
    ha_track_list(entity_id, "available_modes", slot.available_modes);
    ha_track_float(entity_id, "humidity", slot.target_humidity);
    ha_track_float(entity_id, "current_humidity", slot.current_humidity);
    ha_track_state(entity_id, "action", slot.action);
    // Home Assistant rejects a target outside these.
    ha_track_float(entity_id, "min_humidity", slot.min_humidity);
    ha_track_float(entity_id, "max_humidity", slot.max_humidity);
  }

  const std::string &state(int idx) const { return at_(idx, &HumidifierSlot::state, unknown_string()); }
  const std::string &action(int idx) const { return at_(idx, &HumidifierSlot::action, unknown_string()); }
  const std::string &mode(int idx) const { return at_(idx, &HumidifierSlot::mode); }
  const std::string &available_modes(int idx) const { return at_(idx, &HumidifierSlot::available_modes); }
  float target_humidity(int idx) const { return at_(idx, &HumidifierSlot::target_humidity); }
  float current_humidity(int idx) const { return at_(idx, &HumidifierSlot::current_humidity); }
  float min_humidity(int idx) const { return at_(idx, &HumidifierSlot::min_humidity); }
  float max_humidity(int idx) const { return at_(idx, &HumidifierSlot::max_humidity); }
};

struct ClimateSlot {
  std::string state;
  std::string last_active_mode;
  std::string hvac_action;
  std::string hvac_modes;
  std::string fan_mode;
  std::string fan_modes;
  std::string preset_mode;
  std::string preset_modes;
  std::string swing_mode;
  std::string swing_modes;
  float target_temperature = NAN;
  float target_temperature_low = NAN;
  float target_temperature_high = NAN;
  float current_temperature = NAN;
  float target_humidity = NAN;
  float min_temperature = NAN;
  float max_temperature = NAN;
  float min_humidity = NAN;
  float max_humidity = NAN;
};

class ClimateStatusTracker : public EntityTracker<ClimateSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 18;

  ClimateStatusTracker() : EntityTracker(REMOTE_MODE_CLIMATE) {}

  // There is no hvac_mode attribute: a climate entity's state IS its HVAC mode.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ClimateSlot &slot = this->slots_[idx];
    ha_subscribe(entity_id, nullptr, [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    ha_track_float(entity_id, "temperature", slot.target_temperature);
    ha_track_float(entity_id, "target_temp_low", slot.target_temperature_low);
    ha_track_float(entity_id, "target_temp_high", slot.target_temperature_high);
    ha_track_float(entity_id, "current_temperature", slot.current_temperature);
    std::string *hvac_action = &slot.hvac_action;
    ha_subscribe(entity_id, "hvac_action", [hvac_action](esphome::StringRef state) {
      ha_assign_state_or_unknown(*hvac_action, state);
      if (!ha_state_missing(*hvac_action)) {
        for (auto &c : *hvac_action) {
          if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        }
      }
    });
    ha_track_list(entity_id, "hvac_modes", slot.hvac_modes);
    ha_track_text(entity_id, "fan_mode", slot.fan_mode);
    ha_track_list(entity_id, "fan_modes", slot.fan_modes);
    ha_track_float(entity_id, "humidity", slot.target_humidity);
    ha_track_text(entity_id, "preset_mode", slot.preset_mode);
    ha_track_list(entity_id, "preset_modes", slot.preset_modes);
    // Only thermostats that swing their louvres have swing_modes.
    ha_track_text(entity_id, "swing_mode", slot.swing_mode);
    ha_track_list(entity_id, "swing_modes", slot.swing_modes);
    // Home Assistant rejects a setpoint outside these.
    ha_track_float(entity_id, "min_temp", slot.min_temperature);
    ha_track_float(entity_id, "max_temp", slot.max_temperature);
    ha_track_float(entity_id, "min_humidity", slot.min_humidity);
    ha_track_float(entity_id, "max_humidity", slot.max_humidity);
  }

  const std::string &state(int idx) const { return at_(idx, &ClimateSlot::state, unknown_string()); }
  // The last HVAC mode other than "off" this thermostat reported since boot.
  const std::string &last_active_mode(int idx) const { return at_(idx, &ClimateSlot::last_active_mode); }
  const std::string &hvac_action(int idx) const { return at_(idx, &ClimateSlot::hvac_action, unknown_string()); }
  const std::string &hvac_modes(int idx) const { return at_(idx, &ClimateSlot::hvac_modes); }
  const std::string &fan_mode(int idx) const { return at_(idx, &ClimateSlot::fan_mode); }
  const std::string &fan_modes(int idx) const { return at_(idx, &ClimateSlot::fan_modes); }
  const std::string &preset_mode(int idx) const { return at_(idx, &ClimateSlot::preset_mode); }
  const std::string &preset_modes(int idx) const { return at_(idx, &ClimateSlot::preset_modes); }
  bool supports_preset(int idx) const { return !this->preset_modes(idx).empty(); }
  const std::string &swing_mode(int idx) const { return at_(idx, &ClimateSlot::swing_mode); }
  const std::string &swing_modes(int idx) const { return at_(idx, &ClimateSlot::swing_modes); }
  float target_humidity(int idx) const { return at_(idx, &ClimateSlot::target_humidity); }
  float target_temperature(int idx) const { return at_(idx, &ClimateSlot::target_temperature); }
  float target_temperature_low(int idx) const { return at_(idx, &ClimateSlot::target_temperature_low); }
  float target_temperature_high(int idx) const { return at_(idx, &ClimateSlot::target_temperature_high); }
  float current_temperature(int idx) const { return at_(idx, &ClimateSlot::current_temperature); }
  float min_temperature(int idx) const { return at_(idx, &ClimateSlot::min_temperature); }
  float max_temperature(int idx) const { return at_(idx, &ClimateSlot::max_temperature); }
  float min_humidity(int idx) const { return at_(idx, &ClimateSlot::min_humidity); }
  float max_humidity(int idx) const { return at_(idx, &ClimateSlot::max_humidity); }

 protected:
  void store_state_(int idx, esphome::StringRef state) {
    ClimateSlot &slot = this->slots_[idx];
    ha_assign_state_or_unknown(slot.state, state);
    if (!ha_state_missing(slot.state) && slot.state != "off") {
      slot.last_active_mode = slot.state;
    }
  }
};

struct LockSlot {
  std::string state;
  int supported_features = -1;
};

class LockStatusTracker : public SingleStateTracker<LockSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 2;

  LockStatusTracker() : SingleStateTracker(REMOTE_MODE_LOCKS) {}

  // Whether the lock can be opened (unlatched), not just unlocked.
  void subscribe(int idx) {
    SingleStateTracker::subscribe(idx);
    ha_track_int(this->entity_id(idx), "supported_features", this->slots_[idx].supported_features);
  }

  bool supports_open(int idx) const {
    return in_range_(idx) && this->slots_[idx].supported_features >= 0 &&
           (this->slots_[idx].supported_features & LOCK_FEATURE_OPEN) != 0;
  }
};

struct SensorSlot {
  std::string state;
  std::string unit;
  std::string event_type;
  std::string device_class;
};

class SensorStatusTracker : public SingleStateTracker<SensorSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 3;

  SensorStatusTracker() : SingleStateTracker(REMOTE_MODE_SENSORS) {}

  // suggested_unit_of_measurement is an entity-registry option, not a state
  // attribute, so Home Assistant never sends it; unit_of_measurement already
  // reflects any unit override. Binary sensors, people and device trackers
  // have no unit, so they don't spend a subscription on one. An event's state
  // is when it last happened; its event_type says what happened ("ring") and
  // its device_class what kind of thing it is ("doorbell").
  void subscribe(int idx) {
    SingleStateTracker::subscribe(idx);
    SensorSlot &slot = this->slots_[idx];
    if (this->is_event(idx)) {
      ha_track_text(this->entity_id(idx), "event_type", slot.event_type);
      ha_track_text(this->entity_id(idx), "device_class", slot.device_class);
      return;
    }
    if (entity_id_matches_domain(this->entity_id(idx), "binary_sensor") || this->is_presence(idx)) {
      return;
    }
    std::string *unit = &slot.unit;
    ha_subscribe(this->entity_id(idx), "unit_of_measurement", [unit](esphome::StringRef state) {
      const char *begin = state.c_str();
      const char *end = begin + state.size();
      while (begin < end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n')) begin++;
      while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) end--;
      if (!ha_state_missing(begin, end - begin)) {
        unit->assign(begin, end - begin);
        normalize_unit_text(*unit);
      }
    });
  }

  const std::string &unit(int idx) const { return at_(idx, &SensorSlot::unit); }
  const std::string &event_type(int idx) const { return at_(idx, &SensorSlot::event_type); }
  const std::string &device_class(int idx) const { return at_(idx, &SensorSlot::device_class); }
  bool is_event(int idx) const { return entity_id_matches_domain(this->entity_id(idx), "event"); }
  // A person or device tracker: home, away or a zone.
  bool is_presence(int idx) const {
    return entity_id_matches_domain(this->entity_id(idx), "person") ||
           entity_id_matches_domain(this->entity_id(idx), "device_tracker");
  }
};

struct CoverSlot {
  std::string state;
  float position = NAN;
  float tilt = NAN;
  int supported_features = 0;
};

class CoverStatusTracker : public EntityTracker<CoverSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 4;

  CoverStatusTracker() : EntityTracker(REMOTE_MODE_COVERS) {}

  // Valves report the same states and feature bits as covers (open 1, close 2,
  // set position 4, stop 8) but have no tilt.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    CoverSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_float(entity_id, "current_position", slot.position);
    if (!this->is_valve(idx)) {
      ha_track_float(entity_id, "current_tilt_position", slot.tilt);
    }
    ha_track_int(entity_id, "supported_features", slot.supported_features);
  }

  const std::string &state(int idx) const { return at_(idx, &CoverSlot::state, unknown_string()); }
  float position(int idx) const { return at_(idx, &CoverSlot::position); }
  bool has_position(int idx) const { return this->has_feature_(idx, 4); }
  float tilt(int idx) const { return at_(idx, &CoverSlot::tilt); }
  bool has_tilt(int idx) const { return this->has_feature_(idx, 128); }
  bool supports_stop(int idx) const { return this->has_feature_(idx, 8); }
  bool is_valve(int idx) const { return entity_id_matches_domain(this->entity_id(idx), "valve"); }

 protected:
  bool has_feature_(int idx, int feature) const {
    return in_range_(idx) && (this->slots_[idx].supported_features & feature) != 0;
  }
};

struct MediaSlot {
  std::string state;
  std::string device_class;
  std::string title;
  std::string artist;
  std::string source;
  std::string source_list;
  std::string shuffle;
  std::string repeat;
  std::string sound_mode;
  std::string sound_mode_list;
  std::string muted;
  float volume = NAN;
};

class MediaStatusTracker : public EntityTracker<MediaSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 12;

  MediaStatusTracker() : EntityTracker(REMOTE_MODE_MEDIA) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    MediaSlot &slot = this->slots_[idx];
    ha_subscribe(entity_id, nullptr, [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    ha_track_text(entity_id, "device_class", slot.device_class);
    ha_track_list(entity_id, "source_list", slot.source_list);
    ha_track_float(entity_id, "volume_level", slot.volume);
    ha_track_text(entity_id, "media_title", slot.title);
    ha_track_text(entity_id, "media_artist", slot.artist);
    ha_subscribe(entity_id, "source", [this, idx](esphome::StringRef state) { this->store_source_(idx, state); });
    ha_track_text(entity_id, "shuffle", slot.shuffle);
    ha_track_text(entity_id, "repeat", slot.repeat);
    ha_track_text(entity_id, "sound_mode", slot.sound_mode);
    ha_track_list(entity_id, "sound_mode_list", slot.sound_mode_list);
    ha_track_text(entity_id, "is_volume_muted", slot.muted);
  }

  const std::string &state(int idx) const { return at_(idx, &MediaSlot::state, unknown_string()); }
  const std::string &title(int idx) const { return at_(idx, &MediaSlot::title); }
  const std::string &device_class(int idx) const { return at_(idx, &MediaSlot::device_class); }
  const std::string &artist(int idx) const { return at_(idx, &MediaSlot::artist); }
  const std::string &source(int idx) const { return at_(idx, &MediaSlot::source); }
  const std::string &source_list(int idx) const { return at_(idx, &MediaSlot::source_list); }
  float volume(int idx) const { return at_(idx, &MediaSlot::volume); }
  const std::string &shuffle(int idx) const { return at_(idx, &MediaSlot::shuffle); }
  const std::string &repeat(int idx) const { return at_(idx, &MediaSlot::repeat); }
  const std::string &sound_mode(int idx) const { return at_(idx, &MediaSlot::sound_mode); }
  const std::string &sound_mode_list(int idx) const { return at_(idx, &MediaSlot::sound_mode_list); }
  // "True"/"False", or empty for a player that doesn't report it.
  const std::string &muted(int idx) const { return at_(idx, &MediaSlot::muted); }

 protected:
  // Home Assistant drops a player's attributes when it stops or turns off, and
  // sends nothing for a dropped attribute, so clear them here or the last
  // track, volume and playback settings stay on screen. They come back once the
  // player reports them again. (A player that keeps its track while idle
  // doesn't resend it when that track resumes; that can't be told apart from
  // one that dropped it, and a blank title beats a wrong one.)
  void store_state_(int idx, esphome::StringRef state) {
    MediaSlot &slot = this->slots_[idx];
    ha_assign_state_or_unknown(slot.state, state);
    const std::string &current = slot.state;
    if (current != "playing" && current != "paused" && current != "buffering" && current != "on") {
      slot.title.clear();
      slot.artist.clear();
    }
    if (current == "off" || ha_state_missing(current)) {
      slot.shuffle.clear();
      slot.repeat.clear();
      slot.sound_mode.clear();
      slot.muted.clear();
      slot.volume = NAN;
    }
  }

  void store_source_(int idx, esphome::StringRef state) {
    MediaSlot &slot = this->slots_[idx];
    if (slot.device_class == "tv" || slot.device_class == "receiver") {
      // TVs/receivers briefly report an empty source during switching; keep the last one.
      size_t i = 0;
      while (i < state.size() && (state[i] == ' ' || state[i] == '\t' || state[i] == '\r' || state[i] == '\n')) {
        i++;
      }
      if (i == state.size()) {
        return;
      }
    }
    ha_assign(slot.source, state);
  }
};

struct WaterHeaterSlot {
  std::string state;
  std::string last_active_mode;
  std::string operation_mode;
  std::string operation_list;
  std::string away_mode;
  int supported_features = -1;
  float target_temperature = NAN;
  float min_temperature = NAN;
  float max_temperature = NAN;
};

class WaterHeaterStatusTracker : public EntityTracker<WaterHeaterSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 8;

  WaterHeaterStatusTracker() : EntityTracker(REMOTE_MODE_WATER_HEATERS) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    WaterHeaterSlot &slot = this->slots_[idx];
    ha_subscribe(entity_id, nullptr, [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    ha_track_float(entity_id, "temperature", slot.target_temperature);
    // Home Assistant doesn't range-check water_heater.set_temperature, so the
    // remote clamps to the heater's own limits.
    ha_track_float(entity_id, "min_temp", slot.min_temperature);
    ha_track_float(entity_id, "max_temp", slot.max_temperature);
    ha_track_text(entity_id, "operation_mode", slot.operation_mode);
    ha_track_list(entity_id, "operation_list", slot.operation_list);
    ha_track_text(entity_id, "away_mode", slot.away_mode);
    // water_heater.turn_on/turn_off only work on heaters with the on/off feature.
    ha_track_int(entity_id, "supported_features", slot.supported_features);
  }

  const std::string &state(int idx) const { return at_(idx, &WaterHeaterSlot::state, unknown_string()); }
  // The last operation other than "off" this heater reported since boot.
  const std::string &last_active_mode(int idx) const { return at_(idx, &WaterHeaterSlot::last_active_mode); }
  float target_temperature(int idx) const { return at_(idx, &WaterHeaterSlot::target_temperature); }
  float min_temperature(int idx) const { return at_(idx, &WaterHeaterSlot::min_temperature); }
  float max_temperature(int idx) const { return at_(idx, &WaterHeaterSlot::max_temperature); }
  const std::string &operation_mode(int idx) const { return at_(idx, &WaterHeaterSlot::operation_mode); }
  const std::string &operation_list(int idx) const { return at_(idx, &WaterHeaterSlot::operation_list); }
  const std::string &away_mode(int idx) const { return at_(idx, &WaterHeaterSlot::away_mode); }
  // -1 until Home Assistant has sent the heater's features.
  int supported_features(int idx) const { return in_range_(idx) ? this->slots_[idx].supported_features : -1; }

 protected:
  void store_state_(int idx, esphome::StringRef state) {
    WaterHeaterSlot &slot = this->slots_[idx];
    ha_assign_state_or_unknown(slot.state, state);
    if (!ha_state_missing(slot.state) && slot.state != "off") {
      slot.last_active_mode = slot.state;
    }
  }
};

// Automations report on/off for enabled/disabled, scripts "on" while they
// run, and a scene the time it was last activated.
struct AutomationSlot {
  std::string state;
  std::string last_triggered;
  std::string mode;
};

class AutomationStatusTracker : public SingleStateTracker<AutomationSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 2;

  AutomationStatusTracker() : SingleStateTracker(REMOTE_MODE_AUTOMATION) {}

  // Home Assistant answers automation.trigger only once the whole run ends, so
  // last_triggered is what shows it ran. A script's mode says whether starting
  // it again while it runs does anything ("single" ignores it).
  void subscribe(int idx) {
    SingleStateTracker::subscribe(idx);
    const char *entity_id = this->entity_id(idx);
    if (entity_id_matches_domain(entity_id, "automation")) {
      ha_track_text(entity_id, "last_triggered", this->slots_[idx].last_triggered);
    } else if (entity_id_matches_domain(entity_id, "script")) {
      ha_track_text(entity_id, "mode", this->slots_[idx].mode);
    }
  }

  const std::string &last_triggered(int idx) const { return at_(idx, &AutomationSlot::last_triggered); }
  const std::string &mode(int idx) const { return at_(idx, &AutomationSlot::mode); }
};

struct AlarmSlot {
  std::string state;
  int supported_features = -1;
};

class AlarmStatusTracker : public SingleStateTracker<AlarmSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 2;

  AlarmStatusTracker() : SingleStateTracker(REMOTE_MODE_ALARMS) {}

  // Which arm modes the panel supports, and whether it can be triggered.
  void subscribe(int idx) {
    SingleStateTracker::subscribe(idx);
    ha_track_int(this->entity_id(idx), "supported_features", this->slots_[idx].supported_features);
  }

  // -1 until Home Assistant has sent them.
  int supported_features(int idx) const { return in_range_(idx) ? this->slots_[idx].supported_features : -1; }
};

// sun.sun says whether it is night, for the weather icons that have a night
// form. Subscribed only when there is a weather favorite.
class SunStateTracker {
 public:
  void subscribe() { ha_track_state("sun.sun", nullptr, this->state_); }

  // 1 below the horizon, 0 above it, -1 unknown (no sun integration).
  int below_horizon() const {
    return this->state_ == "below_horizon" ? 1 : this->state_ == "above_horizon" ? 0 : -1;
  }

 private:
  std::string state_;
};

// A weather.get_forecasts request (see WeatherStatusTracker).
struct ForecastRequest {
  uint32_t sent_at = 0;
  uint8_t sent = 0;
  bool twice_daily = false;
  bool done = false;
};

struct WeatherSlot {
  ForecastRequest forecast;
  std::string state;
  std::string temperature_unit;
  std::string wind_speed_unit;
  std::string pressure_unit;
  std::string precipitation_unit;
  float temperature = NAN;
  float humidity = NAN;
  float high_temperature = NAN;
  float low_temperature = NAN;
  float wind_speed = NAN;
  float wind_bearing = NAN;
  float wind_gust_speed = NAN;
  float pressure = NAN;
  float cloud_coverage = NAN;
  float uv_index = NAN;
  float dew_point = NAN;
  float apparent_temperature = NAN;
  float precipitation = NAN;
};

class WeatherStatusTracker : public EntityTracker<WeatherSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 15;

  WeatherStatusTracker() : EntityTracker(REMOTE_MODE_WEATHER) {}

  // The unit attributes come first: every value below is in the unit the
  // weather entity reports, which Home Assistant can set per entity.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    WeatherSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_text(entity_id, "temperature_unit", slot.temperature_unit);
    ha_track_text(entity_id, "wind_speed_unit", slot.wind_speed_unit);
    ha_track_text(entity_id, "pressure_unit", slot.pressure_unit);
    ha_track_text(entity_id, "precipitation_unit", slot.precipitation_unit);
    ha_track_float(entity_id, "temperature", slot.temperature);
    ha_track_float(entity_id, "humidity", slot.humidity);
    ha_track_float(entity_id, "wind_speed", slot.wind_speed);
    // Degrees, or a compass point ("NW") from some integrations.
    float *bearing = &slot.wind_bearing;
    ha_subscribe(entity_id, "wind_bearing", [bearing](esphome::StringRef state) {
      float value = ha_parse_float(state);
      *bearing = std::isnan(value) ? compass_point_degrees(state.c_str(), state.size()) : value;
    });
    ha_track_float(entity_id, "wind_gust_speed", slot.wind_gust_speed);
    ha_track_float(entity_id, "pressure", slot.pressure);
    ha_track_float(entity_id, "cloud_coverage", slot.cloud_coverage);
    ha_track_float(entity_id, "uv_index", slot.uv_index);
    ha_track_float(entity_id, "dew_point", slot.dew_point);
    ha_track_float(entity_id, "apparent_temperature", slot.apparent_temperature);
  }

  const std::string &state(int idx) const { return at_(idx, &WeatherSlot::state, unknown_string()); }
  float temperature(int idx) const { return at_(idx, &WeatherSlot::temperature); }
  float humidity(int idx) const { return at_(idx, &WeatherSlot::humidity); }
  float high_temperature(int idx) const { return at_(idx, &WeatherSlot::high_temperature); }
  float low_temperature(int idx) const { return at_(idx, &WeatherSlot::low_temperature); }
  float wind_speed(int idx) const { return at_(idx, &WeatherSlot::wind_speed); }
  float wind_bearing(int idx) const { return at_(idx, &WeatherSlot::wind_bearing); }
  float wind_gust_speed(int idx) const { return at_(idx, &WeatherSlot::wind_gust_speed); }
  float pressure(int idx) const { return at_(idx, &WeatherSlot::pressure); }
  float cloud_coverage(int idx) const { return at_(idx, &WeatherSlot::cloud_coverage); }
  float uv_index(int idx) const { return at_(idx, &WeatherSlot::uv_index); }
  float dew_point(int idx) const { return at_(idx, &WeatherSlot::dew_point); }
  float apparent_temperature(int idx) const { return at_(idx, &WeatherSlot::apparent_temperature); }
  float precipitation(int idx) const { return at_(idx, &WeatherSlot::precipitation); }
  const std::string &temperature_unit(int idx) const { return at_(idx, &WeatherSlot::temperature_unit); }
  const std::string &wind_speed_unit(int idx) const { return at_(idx, &WeatherSlot::wind_speed_unit); }
  const std::string &pressure_unit(int idx) const { return at_(idx, &WeatherSlot::pressure_unit); }
  const std::string &precipitation_unit(int idx) const { return at_(idx, &WeatherSlot::precipitation_unit); }

  // Today's high, low and precipitation come from weather.get_forecasts
  // (fetch_weather_forecast): Home Assistant dropped the forecast attribute
  // from weather entities in 2024.3. Asked for once per wake: the daily
  // forecast, or the twice-daily one from an integration without a daily
  // forecast (the US National Weather Service). A request Home Assistant
  // drops, so never answers, is sent again after a while.
  static constexpr uint32_t FORECAST_ANSWER_WAIT_MS = 20000;
  static constexpr uint8_t FORECAST_MAX_REQUESTS = 4;

  bool forecast_due(int idx, uint32_t now) const {
    if (!in_range_(idx)) {
      return false;
    }
    const ForecastRequest &request = this->slots_[idx].forecast;
    return !request.done && request.sent < FORECAST_MAX_REQUESTS &&
           (request.sent_at == 0 || now - request.sent_at >= FORECAST_ANSWER_WAIT_MS);
  }
  const char *forecast_type(int idx) const {
    return in_range_(idx) && this->slots_[idx].forecast.twice_daily ? "twice_daily" : "daily";
  }
  void mark_forecast_sent(int idx, uint32_t now) {
    if (in_range_(idx)) {
      this->slots_[idx].forecast.sent_at = now | 1;  // 0 means "not sent"
      this->slots_[idx].forecast.sent++;
    }
  }
  // Home Assistant refused the request: one for daily forecasts from an
  // integration without them is asked again for twice-daily ones.
  void forecast_failed(int idx) {
    if (!in_range_(idx)) {
      return;
    }
    ForecastRequest &request = this->slots_[idx].forecast;
    if (request.twice_daily) {
      request.done = true;
    } else {
      request.twice_daily = true;
      request.sent_at = 0;
    }
  }
  void store_forecast(int idx, float high, float low, float precipitation) {
    if (in_range_(idx)) {
      WeatherSlot &slot = this->slots_[idx];
      slot.high_temperature = high;
      slot.low_temperature = low;
      slot.precipitation = precipitation;
      slot.forecast.done = true;
    }
  }
};

// Number and select entities, and their input_number and input_select helpers.
struct InputSlot {
  std::string state;
  std::string options;
  std::string unit;
  float min = NAN;
  float max = NAN;
  float step = NAN;
};

class InputStatusTracker : public EntityTracker<InputSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 5;

  InputStatusTracker() : EntityTracker(REMOTE_MODE_INPUTS) {}

  // A number's range, step and unit; a select's options.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    InputSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    if (this->is_select(idx)) {
      ha_track_list(entity_id, "options", slot.options);
      return;
    }
    ha_track_float(entity_id, "min", slot.min);
    ha_track_float(entity_id, "max", slot.max);
    ha_track_float(entity_id, "step", slot.step);
    std::string *unit = &slot.unit;
    ha_subscribe(entity_id, "unit_of_measurement", [unit](esphome::StringRef state) {
      if (!ha_state_missing(state)) {
        unit->assign(state.c_str(), state.size());
        normalize_unit_text(*unit);
      }
    });
  }

  bool is_select(int idx) const {
    return entity_id_matches_domain(this->entity_id(idx), "select") ||
           entity_id_matches_domain(this->entity_id(idx), "input_select");
  }
  const std::string &state(int idx) const { return at_(idx, &InputSlot::state, unknown_string()); }
  const std::string &options(int idx) const { return at_(idx, &InputSlot::options); }
  const std::string &unit(int idx) const { return at_(idx, &InputSlot::unit); }
  float min(int idx) const { return at_(idx, &InputSlot::min); }
  float max(int idx) const { return at_(idx, &InputSlot::max); }
  float step(int idx) const { return at_(idx, &InputSlot::step); }
};

// Vacuums and lawn mowers. Only vacuums have fan speeds. Home Assistant has
// deprecated the vacuum battery attributes in favour of a separate battery
// sensor, so none is followed here (add that sensor as a favorite instead).
struct VacuumSlot {
  std::string state;
  std::string fan_speed;
  std::string fan_speed_list;
};

class VacuumStatusTracker : public EntityTracker<VacuumSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 3;

  VacuumStatusTracker() : EntityTracker(REMOTE_MODE_VACUUMS) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    VacuumSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    if (this->is_mower(idx)) {
      return;
    }
    ha_track_text(entity_id, "fan_speed", slot.fan_speed);
    ha_track_list(entity_id, "fan_speed_list", slot.fan_speed_list);
  }

  bool is_mower(int idx) const { return entity_id_matches_domain(this->entity_id(idx), "lawn_mower"); }
  const std::string &state(int idx) const { return at_(idx, &VacuumSlot::state, unknown_string()); }
  const std::string &fan_speed(int idx) const { return at_(idx, &VacuumSlot::fan_speed); }
  const std::string &fan_speed_list(int idx) const { return at_(idx, &VacuumSlot::fan_speed_list); }
};

// Timers: idle, active or paused. An active timer reports when it finishes;
// a paused one how long it had left; an idle one its duration.
struct TimerSlot {
  std::string state;
  std::string duration;
  std::string remaining;
  std::string finishes_at;
};

class TimerStatusTracker : public EntityTracker<TimerSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 4;

  TimerStatusTracker() : EntityTracker(REMOTE_MODE_TIMERS) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    TimerSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_text(entity_id, "duration", slot.duration);
    ha_track_text(entity_id, "remaining", slot.remaining);
    ha_track_text(entity_id, "finishes_at", slot.finishes_at);
  }

  const std::string &state(int idx) const { return at_(idx, &TimerSlot::state, unknown_string()); }

  // Seconds left, as of now_epoch (seconds since 1970, 0 when the clock isn't
  // set); -1 when it isn't known yet.
  int64_t seconds_left(int idx, int64_t now_epoch) const {
    if (!in_range_(idx)) {
      return -1;
    }
    const TimerSlot &slot = this->slots_[idx];
    int64_t seconds = 0;
    if (slot.state == "active" && now_epoch > 0) {
      int64_t finishes = 0;
      bool date_only = false;
      if (parse_ha_timestamp(slot.finishes_at, &finishes, &date_only) && !date_only) {
        return finishes > now_epoch ? finishes - now_epoch : 0;
      }
    }
    if (slot.state != "idle" && parse_ha_duration(slot.remaining, &seconds)) {
      return seconds;
    }
    if (parse_ha_duration(slot.duration, &seconds)) {
      return seconds;
    }
    return -1;
  }
};

// TV and streaming-box remotes. Only some report activities: a Harmony hub's,
// or the apps set up in the Android TV Remote integration. A Harmony hub
// reports "PowerOff" as its activity while everything is off.
struct RemoteSlot {
  std::string state;
  std::string activity;
  std::string activity_list;
};

class RemoteStatusTracker : public EntityTracker<RemoteSlot> {
 public:
  static constexpr int SUBSCRIPTIONS = 3;

  RemoteStatusTracker() : EntityTracker(REMOTE_MODE_REMOTES) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    RemoteSlot &slot = this->slots_[idx];
    ha_track_state(entity_id, nullptr, slot.state);
    ha_track_text(entity_id, "current_activity", slot.activity);
    ha_track_list(entity_id, "activity_list", slot.activity_list);
  }

  const std::string &state(int idx) const { return at_(idx, &RemoteSlot::state, unknown_string()); }
  // Empty when there is none, or everything is off.
  const std::string &activity(int idx) const {
    const std::string &activity = at_(idx, &RemoteSlot::activity);
    return activity == "PowerOff" || ha_state_missing(activity) ? empty_string() : activity;
  }
  const std::string &activity_list(int idx) const { return at_(idx, &RemoteSlot::activity_list); }
  // The favorite's third field: a command set, or none.
  const char *commands(int idx) const { return mode_item_sources_cstr(REMOTE_MODE_REMOTES, idx); }
};

class NotificationFeedTracker {
 public:
  void subscribe() {
    if (notification_feed_entity_()[0] == '\0') {
      return;
    }
    ha_subscribe(notification_feed_entity_(), notification_feed_attribute_(),
                 [this](esphome::StringRef state) { this->on_payload_(state); });
    ha_subscribe(notification_feed_entity_(), notification_feed_ids_attribute_(),
                 [this](esphome::StringRef state) { this->on_ids_payload_(state); });
  }

  int item_count() const { return this->count_ > 0 ? this->count_ : 1; }

  void label_to_buffer(int idx, char *buffer, size_t buffer_size) const {
    if (buffer == nullptr || buffer_size == 0) {
      return;
    }
    if (this->count_ <= 0) {
      buffer[0] = '\0';  // NO ALERTS HEADER
      return;
    }
    // Both values are bounded by NOTIFICATION_FEED_MAX_ITEMS, so the longest
    // possible label is "16 OF 16". Clamping again here is redundant at runtime
    // but lets the compiler see the range and drop its truncation warning.
    const int count = this->count_ > NOTIFICATION_FEED_MAX_ITEMS ? NOTIFICATION_FEED_MAX_ITEMS : this->count_;
    idx = clamp_mode_index(idx, count);
    snprintf(buffer, buffer_size, "%d OF %d", idx + 1, count);
  }

  std::string label(int idx) const {
    char buffer[24];
    buffer[0] = '\0';
    this->label_to_buffer(idx, buffer, sizeof(buffer));
    return std::string(buffer);
  }

  static const char *entity_cstr() { return notification_feed_entity_(); }

  std::string entity() const { return notification_feed_entity_(); }

  const std::string &message(int idx) const {
    if (this->count_ <= 0) {
      return empty_string();
    }
    idx = clamp_mode_index(idx, this->count_);
    return this->messages_[idx];
  }

  const std::string &notification_id(int idx) const {
    if (this->count_ <= 0) {
      return empty_string();
    }
    idx = clamp_mode_index(idx, this->count_);
    return this->ids_[idx];
  }

 private:
  static const char *notification_feed_entity_() {
    return NOTIFICATION_FEED_ENTITY;
  }

  static const char *notification_feed_attribute_() {
    return (NOTIFICATION_FEED_ATTRIBUTE[0] == '\0') ? "messages" : NOTIFICATION_FEED_ATTRIBUTE;
  }

  static const char *notification_feed_ids_attribute_() {
    return (NOTIFICATION_FEED_IDS_ATTRIBUTE[0] == '\0') ? "ids" : NOTIFICATION_FEED_IDS_ATTRIBUTE;
  }

  static std::string notification_feed_separator_() {
    std::string separator = NOTIFICATION_FEED_SEPARATOR;
    return separator.empty() ? std::string("||") : separator;
  }

  // Splits payload into positional slots. Interior empty items keep their slot
  // so that the messages and ids attributes stay index-aligned even when one
  // item is empty; only trailing empties are dropped. Returns the slot count.
  static int parse_delimited_slots_(const std::string &payload, std::string *slots) {
    for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
      slots[i].clear();
    }
    if (ha_state_missing(payload) || payload == "[]" || payload == "none") {
      return 0;
    }

    const std::string separator = notification_feed_separator_();
    size_t start = 0;
    int count = 0;
    while (count < NOTIFICATION_FEED_MAX_ITEMS) {
      size_t end = payload.find(separator, start);
      slots[count++] = trim_copy(payload.substr(start, end == std::string::npos ? std::string::npos : end - start));
      if (end == std::string::npos) {
        break;
      }
      start = end + separator.size();
    }
    while (count > 0 && slots[count - 1].empty()) {
      count--;
    }
    return count;
  }

  // Bounds the payload before it is copied. Parsing works on a full copy plus a
  // copy per slot, so an oversized attribute would cost several times its size
  // in heap; the JSON list parsers apply the same cap.
  static bool payload_too_large_(esphome::StringRef state, const char *attribute) {
    if (state.size() <= REMOTE_HA_MAX_JSON_PAYLOAD_BYTES) {
      return false;
    }
    ESP_LOGW("remote_config", "Ignoring oversized notification %s payload (%u bytes)", attribute,
             static_cast<unsigned>(state.size()));
    return true;
  }

  void on_payload_(esphome::StringRef state) {
    if (payload_too_large_(state, notification_feed_attribute_())) {
      this->count_ = 0;
      for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
        this->messages_[i].clear();
      }
      return;
    }
    std::string payload = trim_copy(state.str());
    this->count_ = parse_delimited_slots_(payload, this->messages_);
  }

  void on_ids_payload_(esphome::StringRef state) {
    if (payload_too_large_(state, notification_feed_ids_attribute_())) {
      for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
        this->ids_[i].clear();
      }
      return;
    }
    std::string payload = trim_copy(state.str());
    parse_delimited_slots_(payload, this->ids_);
  }

  int count_{0};
  std::string messages_[NOTIFICATION_FEED_MAX_ITEMS];
  std::string ids_[NOTIFICATION_FEED_MAX_ITEMS];
};
