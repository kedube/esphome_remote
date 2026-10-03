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
// the constexpr favorite lists and attribute names are string literals, so the
// server can keep the pointers as-is, and every callback below captures at most
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

// Common base: binds a tracker to its domain's entity list and provides
// bounds-checked accessors. Subclasses implement subscribe(idx), which
// registers every subscription for one entity.
template <int Count>
class EntityTracker {
 public:
  static constexpr int COUNT = Count;

  explicit EntityTracker(const EntityEntry *entities) : entities_(entities) {}

  const char *entity_id(int idx) const { return this->entities_[idx].entity_id; }

 protected:
  static bool in_range_(int idx) { return idx >= 0 && idx < Count; }

  static const std::string &at_(const std::array<std::string, Count> &values, int idx,
                                const std::string &fallback = empty_string()) {
    return in_range_(idx) ? values[idx] : fallback;
  }

  static float at_(const std::array<float, Count> &values, int idx) { return in_range_(idx) ? values[idx] : NAN; }

  const EntityEntry *entities_;
};

template <int Count>
class SingleStateTracker : public EntityTracker<Count> {
 public:
  explicit SingleStateTracker(const EntityEntry *entities) : EntityTracker<Count>(entities) {}

  void subscribe(int idx) { ha_track_state(this->entity_id(idx), nullptr, this->state_[idx]); }

  const std::string &state(int idx) const { return this->at_(this->state_, idx, unknown_string()); }

 protected:
  std::array<std::string, Count> state_{};
};

class LightStatusTracker : public EntityTracker<LIGHT_LIST_COUNT> {
 public:
  LightStatusTracker() : EntityTracker(LIGHT_LIST) {}

  // supported_color_modes is the only reliable sign of a light that can't dim:
  // brightness is missing from every light that is off, and from one that
  // hasn't synced yet.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_track_state(entity_id, nullptr, this->state_[idx]);
    ha_track_float(entity_id, "brightness", this->brightness_[idx]);
    ha_track_list(entity_id, "supported_color_modes", this->color_modes_[idx]);
    ha_track_text(entity_id, "effect", this->effect_[idx]);
    ha_track_list(entity_id, "effect_list", this->effect_list_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  float brightness(int idx) const { return at_(this->brightness_, idx); }
  bool has_brightness(int idx) const { return !std::isnan(this->brightness(idx)); }
  // -1 until the color modes arrive, 0 for an on/off-only light, 1 if it dims.
  int dimmable(int idx) const { return light_modes_dimmable(at_(this->color_modes_, idx)); }
  const std::string &effect(int idx) const { return at_(this->effect_, idx); }
  const std::string &effect_list(int idx) const { return at_(this->effect_list_, idx); }
  bool has_effect(int idx) const { return !this->effect_list(idx).empty(); }

 protected:
  std::array<std::string, LIGHT_LIST_COUNT> state_{};
  std::array<std::string, LIGHT_LIST_COUNT> color_modes_{};
  std::array<std::string, LIGHT_LIST_COUNT> effect_{};
  std::array<std::string, LIGHT_LIST_COUNT> effect_list_{};
  std::array<float, LIGHT_LIST_COUNT> brightness_ = filled_array<float, LIGHT_LIST_COUNT>(NAN);
};

using SwitchStatusTracker = SingleStateTracker<SWITCH_LIST_COUNT>;

class FanStatusTracker : public EntityTracker<FAN_LIST_COUNT> {
 public:
  FanStatusTracker() : EntityTracker(FAN_LIST) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_track_state(entity_id, nullptr, this->state_[idx]);
    ha_track_float(entity_id, "percentage", this->percentage_[idx]);
    ha_track_float(entity_id, "percentage_step", this->percentage_step_[idx]);
    ha_track_text(entity_id, "preset_mode", this->preset_mode_[idx]);
    ha_track_list(entity_id, "preset_modes", this->preset_modes_[idx]);
    ha_track_text(entity_id, "oscillating", this->oscillating_[idx]);
    ha_track_text(entity_id, "direction", this->direction_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  float percentage(int idx) const { return at_(this->percentage_, idx); }
  bool has_percentage(int idx) const { return !std::isnan(this->percentage(idx)); }
  float percentage_step(int idx) const { return at_(this->percentage_step_, idx); }
  // Home Assistant reports percentage and percentage_step for every fan with
  // speeds, but many report the percentage as None while off: the step says
  // the fan has speeds before a percentage arrives.
  bool supports_speed(int idx) const { return this->has_percentage(idx) || !std::isnan(this->percentage_step(idx)); }
  const std::string &preset_mode(int idx) const { return at_(this->preset_mode_, idx); }
  const std::string &preset_modes(int idx) const { return at_(this->preset_modes_, idx); }
  const std::string &oscillating(int idx) const { return at_(this->oscillating_, idx); }
  const std::string &direction(int idx) const { return at_(this->direction_, idx); }

 protected:
  std::array<std::string, FAN_LIST_COUNT> state_{};
  std::array<std::string, FAN_LIST_COUNT> preset_mode_{};
  std::array<std::string, FAN_LIST_COUNT> preset_modes_{};
  std::array<std::string, FAN_LIST_COUNT> oscillating_{};
  std::array<std::string, FAN_LIST_COUNT> direction_{};
  std::array<float, FAN_LIST_COUNT> percentage_ = filled_array<float, FAN_LIST_COUNT>(NAN);
  std::array<float, FAN_LIST_COUNT> percentage_step_ = filled_array<float, FAN_LIST_COUNT>(NAN);
};

class HumidifierStatusTracker : public EntityTracker<HUMIDIFIER_LIST_COUNT> {
 public:
  HumidifierStatusTracker() : EntityTracker(HUMIDIFIER_LIST) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_track_state(entity_id, nullptr, this->state_[idx]);
    ha_track_text(entity_id, "mode", this->mode_[idx]);
    ha_track_list(entity_id, "available_modes", this->available_modes_[idx]);
    ha_track_float(entity_id, "humidity", this->target_humidity_[idx]);
    ha_track_float(entity_id, "current_humidity", this->current_humidity_[idx]);
    ha_track_state(entity_id, "action", this->action_[idx]);
    // Home Assistant rejects a target outside these.
    ha_track_float(entity_id, "min_humidity", this->min_humidity_[idx]);
    ha_track_float(entity_id, "max_humidity", this->max_humidity_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  const std::string &action(int idx) const { return at_(this->action_, idx, unknown_string()); }
  const std::string &mode(int idx) const { return at_(this->mode_, idx); }
  const std::string &available_modes(int idx) const { return at_(this->available_modes_, idx); }
  float target_humidity(int idx) const { return at_(this->target_humidity_, idx); }
  float current_humidity(int idx) const { return at_(this->current_humidity_, idx); }
  float min_humidity(int idx) const { return at_(this->min_humidity_, idx); }
  float max_humidity(int idx) const { return at_(this->max_humidity_, idx); }

 protected:
  std::array<std::string, HUMIDIFIER_LIST_COUNT> state_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> mode_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> available_modes_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> action_{};
  std::array<float, HUMIDIFIER_LIST_COUNT> target_humidity_ = filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
  std::array<float, HUMIDIFIER_LIST_COUNT> current_humidity_ = filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
  std::array<float, HUMIDIFIER_LIST_COUNT> min_humidity_ = filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
  std::array<float, HUMIDIFIER_LIST_COUNT> max_humidity_ = filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
};

class ClimateStatusTracker : public EntityTracker<CLIMATE_LIST_COUNT> {
 public:
  ClimateStatusTracker() : EntityTracker(CLIMATE_LIST) {}

  // There is no hvac_mode attribute: a climate entity's state IS its HVAC mode.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_subscribe(entity_id, nullptr, [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    ha_track_float(entity_id, "temperature", this->target_temperature_[idx]);
    ha_track_float(entity_id, "target_temp_low", this->target_temperature_low_[idx]);
    ha_track_float(entity_id, "target_temp_high", this->target_temperature_high_[idx]);
    ha_track_float(entity_id, "current_temperature", this->current_temperature_[idx]);
    std::string *hvac_action = &this->hvac_action_[idx];
    ha_subscribe(entity_id, "hvac_action", [hvac_action](esphome::StringRef state) {
      ha_assign_state_or_unknown(*hvac_action, state);
      if (!ha_state_missing(*hvac_action)) {
        for (auto &c : *hvac_action) {
          if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        }
      }
    });
    ha_track_list(entity_id, "hvac_modes", this->hvac_modes_[idx]);
    ha_track_text(entity_id, "fan_mode", this->fan_mode_[idx]);
    ha_track_list(entity_id, "fan_modes", this->fan_modes_[idx]);
    ha_track_float(entity_id, "humidity", this->target_humidity_[idx]);
    ha_track_text(entity_id, "preset_mode", this->preset_mode_[idx]);
    ha_track_list(entity_id, "preset_modes", this->preset_modes_[idx]);
    // Home Assistant rejects a setpoint outside these.
    ha_track_float(entity_id, "min_temp", this->min_temperature_[idx]);
    ha_track_float(entity_id, "max_temp", this->max_temperature_[idx]);
    ha_track_float(entity_id, "min_humidity", this->min_humidity_[idx]);
    ha_track_float(entity_id, "max_humidity", this->max_humidity_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  // The last HVAC mode other than "off" this thermostat reported since boot.
  const std::string &last_active_mode(int idx) const { return at_(this->last_active_mode_, idx); }
  const std::string &hvac_action(int idx) const { return at_(this->hvac_action_, idx, unknown_string()); }
  const std::string &hvac_modes(int idx) const { return at_(this->hvac_modes_, idx); }
  const std::string &fan_mode(int idx) const { return at_(this->fan_mode_, idx); }
  const std::string &fan_modes(int idx) const { return at_(this->fan_modes_, idx); }
  const std::string &preset_mode(int idx) const { return at_(this->preset_mode_, idx); }
  const std::string &preset_modes(int idx) const { return at_(this->preset_modes_, idx); }
  bool supports_preset(int idx) const { return !this->preset_modes(idx).empty(); }
  float target_humidity(int idx) const { return at_(this->target_humidity_, idx); }
  float target_temperature(int idx) const { return at_(this->target_temperature_, idx); }
  float target_temperature_low(int idx) const { return at_(this->target_temperature_low_, idx); }
  float target_temperature_high(int idx) const { return at_(this->target_temperature_high_, idx); }
  float current_temperature(int idx) const { return at_(this->current_temperature_, idx); }
  float min_temperature(int idx) const { return at_(this->min_temperature_, idx); }
  float max_temperature(int idx) const { return at_(this->max_temperature_, idx); }
  float min_humidity(int idx) const { return at_(this->min_humidity_, idx); }
  float max_humidity(int idx) const { return at_(this->max_humidity_, idx); }

 protected:
  void store_state_(int idx, esphome::StringRef state) {
    std::string &value = this->state_[idx];
    ha_assign_state_or_unknown(value, state);
    if (!ha_state_missing(value) && value != "off") {
      this->last_active_mode_[idx] = value;
    }
  }

  std::array<std::string, CLIMATE_LIST_COUNT> state_{};
  std::array<std::string, CLIMATE_LIST_COUNT> last_active_mode_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_action_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_modes_{};
  std::array<std::string, CLIMATE_LIST_COUNT> fan_mode_{};
  std::array<std::string, CLIMATE_LIST_COUNT> fan_modes_{};
  std::array<std::string, CLIMATE_LIST_COUNT> preset_mode_{};
  std::array<std::string, CLIMATE_LIST_COUNT> preset_modes_{};
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_low_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_high_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> current_temperature_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_humidity_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> min_temperature_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> max_temperature_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> min_humidity_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> max_humidity_ = filled_array<float, CLIMATE_LIST_COUNT>(NAN);
};

using LockStatusTracker = SingleStateTracker<LOCK_LIST_COUNT>;

class SensorStatusTracker : public SingleStateTracker<SENSOR_LIST_COUNT> {
 public:
  SensorStatusTracker() : SingleStateTracker(SENSOR_LIST) {}

  // suggested_unit_of_measurement is an entity-registry option, not a state
  // attribute, so Home Assistant never sends it; unit_of_measurement already
  // reflects any unit override. Binary sensors have no unit, so they don't
  // spend a subscription on one.
  void subscribe(int idx) {
    SingleStateTracker<SENSOR_LIST_COUNT>::subscribe(idx);
    if (entity_id_matches_domain(this->entity_id(idx), "binary_sensor")) {
      return;
    }
    std::string *unit = &this->unit_[idx];
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

  const std::string &unit(int idx) const { return at_(this->unit_, idx); }

 protected:
  std::array<std::string, SENSOR_LIST_COUNT> unit_{};
};

class CoverStatusTracker : public EntityTracker<COVER_LIST_COUNT> {
 public:
  CoverStatusTracker() : EntityTracker(COVER_LIST) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_track_state(entity_id, nullptr, this->state_[idx]);
    ha_track_float(entity_id, "current_position", this->position_[idx]);
    ha_track_float(entity_id, "current_tilt_position", this->tilt_[idx]);
    ha_track_int(entity_id, "supported_features", this->supported_features_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  float position(int idx) const { return at_(this->position_, idx); }
  bool has_position(int idx) const { return in_range_(idx) && (this->supported_features_[idx] & 4) != 0; }
  float tilt(int idx) const { return at_(this->tilt_, idx); }
  bool has_tilt(int idx) const { return in_range_(idx) && (this->supported_features_[idx] & 128) != 0; }

 protected:
  std::array<std::string, COVER_LIST_COUNT> state_{};
  std::array<float, COVER_LIST_COUNT> position_ = filled_array<float, COVER_LIST_COUNT>(NAN);
  std::array<float, COVER_LIST_COUNT> tilt_ = filled_array<float, COVER_LIST_COUNT>(NAN);
  std::array<int, COVER_LIST_COUNT> supported_features_{};
};

class MediaStatusTracker : public EntityTracker<MEDIA_PLAYER_LIST_COUNT> {
 public:
  MediaStatusTracker() : EntityTracker(MEDIA_PLAYER_LIST) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_subscribe(entity_id, nullptr, [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    ha_track_text(entity_id, "device_class", this->device_class_[idx]);
    ha_track_list(entity_id, "source_list", this->source_list_[idx]);
    ha_track_float(entity_id, "volume_level", this->volume_[idx]);
    ha_track_text(entity_id, "media_title", this->title_[idx]);
    ha_track_text(entity_id, "media_artist", this->artist_[idx]);
    ha_subscribe(entity_id, "source", [this, idx](esphome::StringRef state) { this->store_source_(idx, state); });
    ha_track_text(entity_id, "shuffle", this->shuffle_[idx]);
    ha_track_text(entity_id, "repeat", this->repeat_[idx]);
    ha_track_text(entity_id, "sound_mode", this->sound_mode_[idx]);
    ha_track_list(entity_id, "sound_mode_list", this->sound_mode_list_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  const std::string &title(int idx) const { return at_(this->title_, idx); }
  const std::string &device_class(int idx) const { return at_(this->device_class_, idx); }
  const std::string &artist(int idx) const { return at_(this->artist_, idx); }
  const std::string &source(int idx) const { return at_(this->source_, idx); }
  const std::string &source_list(int idx) const { return at_(this->source_list_, idx); }
  float volume(int idx) const { return at_(this->volume_, idx); }
  const std::string &shuffle(int idx) const { return at_(this->shuffle_, idx); }
  const std::string &repeat(int idx) const { return at_(this->repeat_, idx); }
  const std::string &sound_mode(int idx) const { return at_(this->sound_mode_, idx); }
  const std::string &sound_mode_list(int idx) const { return at_(this->sound_mode_list_, idx); }

 protected:
  // Home Assistant drops a player's attributes when it stops or turns off, and
  // sends nothing for a dropped attribute, so clear them here or the last
  // track, volume and playback settings stay on screen. They come back once the
  // player reports them again. (A player that keeps its track while idle
  // doesn't resend it when that track resumes; that can't be told apart from
  // one that dropped it, and a blank title beats a wrong one.)
  void store_state_(int idx, esphome::StringRef state) {
    ha_assign_state_or_unknown(this->state_[idx], state);
    const std::string &current = this->state_[idx];
    if (current != "playing" && current != "paused" && current != "buffering" && current != "on") {
      this->title_[idx].clear();
      this->artist_[idx].clear();
    }
    if (current == "off" || ha_state_missing(current)) {
      this->shuffle_[idx].clear();
      this->repeat_[idx].clear();
      this->sound_mode_[idx].clear();
      this->volume_[idx] = NAN;
    }
  }

  void store_source_(int idx, esphome::StringRef state) {
    const std::string &device_class = this->device_class_[idx];
    if (device_class == "tv" || device_class == "receiver") {
      // TVs/receivers briefly report an empty source during switching; keep the last one.
      size_t i = 0;
      while (i < state.size() && (state[i] == ' ' || state[i] == '\t' || state[i] == '\r' || state[i] == '\n')) {
        i++;
      }
      if (i == state.size()) {
        return;
      }
    }
    ha_assign(this->source_[idx], state);
  }

  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> state_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> device_class_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> title_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> artist_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> source_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> source_list_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> shuffle_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> repeat_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> sound_mode_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> sound_mode_list_{};
  std::array<float, MEDIA_PLAYER_LIST_COUNT> volume_ = filled_array<float, MEDIA_PLAYER_LIST_COUNT>(NAN);
};

class WaterHeaterStatusTracker : public EntityTracker<WATER_HEATER_LIST_COUNT> {
 public:
  WaterHeaterStatusTracker() : EntityTracker(WATER_HEATER_LIST) {}

  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_subscribe(entity_id, nullptr, [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    ha_track_float(entity_id, "temperature", this->target_temperature_[idx]);
    // Home Assistant doesn't range-check water_heater.set_temperature, so the
    // remote clamps to the heater's own limits.
    ha_track_float(entity_id, "min_temp", this->min_temperature_[idx]);
    ha_track_float(entity_id, "max_temp", this->max_temperature_[idx]);
    ha_track_text(entity_id, "operation_mode", this->operation_mode_[idx]);
    ha_track_list(entity_id, "operation_list", this->operation_list_[idx]);
    ha_track_text(entity_id, "away_mode", this->away_mode_[idx]);
    // water_heater.turn_on/turn_off only work on heaters with the on/off feature.
    ha_track_int(entity_id, "supported_features", this->supported_features_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  // The last operation other than "off" this heater reported since boot.
  const std::string &last_active_mode(int idx) const { return at_(this->last_active_mode_, idx); }
  float target_temperature(int idx) const { return at_(this->target_temperature_, idx); }
  float min_temperature(int idx) const { return at_(this->min_temperature_, idx); }
  float max_temperature(int idx) const { return at_(this->max_temperature_, idx); }
  const std::string &operation_mode(int idx) const { return at_(this->operation_mode_, idx); }
  const std::string &operation_list(int idx) const { return at_(this->operation_list_, idx); }
  const std::string &away_mode(int idx) const { return at_(this->away_mode_, idx); }
  // -1 until Home Assistant has sent the heater's features.
  int supported_features(int idx) const { return in_range_(idx) ? this->supported_features_[idx] : -1; }

 protected:
  void store_state_(int idx, esphome::StringRef state) {
    std::string &value = this->state_[idx];
    ha_assign_state_or_unknown(value, state);
    if (!ha_state_missing(value) && value != "off") {
      this->last_active_mode_[idx] = value;
    }
  }

  std::array<std::string, WATER_HEATER_LIST_COUNT> state_{};
  std::array<std::string, WATER_HEATER_LIST_COUNT> last_active_mode_{};
  std::array<int, WATER_HEATER_LIST_COUNT> supported_features_ = filled_array<int, WATER_HEATER_LIST_COUNT>(-1);
  std::array<std::string, WATER_HEATER_LIST_COUNT> operation_mode_{};
  std::array<std::string, WATER_HEATER_LIST_COUNT> operation_list_{};
  std::array<std::string, WATER_HEATER_LIST_COUNT> away_mode_{};
  std::array<float, WATER_HEATER_LIST_COUNT> target_temperature_ = filled_array<float, WATER_HEATER_LIST_COUNT>(NAN);
  std::array<float, WATER_HEATER_LIST_COUNT> min_temperature_ = filled_array<float, WATER_HEATER_LIST_COUNT>(NAN);
  std::array<float, WATER_HEATER_LIST_COUNT> max_temperature_ = filled_array<float, WATER_HEATER_LIST_COUNT>(NAN);
};

// Automations report on/off for enabled/disabled, scripts "on" while they
// run, and a scene the time it was last activated.
class AutomationStatusTracker : public SingleStateTracker<AUTOMATION_LIST_COUNT> {
 public:
  AutomationStatusTracker() : SingleStateTracker(AUTOMATION_LIST) {}

  // Home Assistant answers automation.trigger only once the whole run ends, so
  // last_triggered is what shows it ran. A script's mode says whether starting
  // it again while it runs does anything ("single" ignores it).
  void subscribe(int idx) {
    SingleStateTracker<AUTOMATION_LIST_COUNT>::subscribe(idx);
    const char *entity_id = this->entity_id(idx);
    if (entity_id_matches_domain(entity_id, "automation")) {
      ha_track_text(entity_id, "last_triggered", this->last_triggered_[idx]);
    } else if (entity_id_matches_domain(entity_id, "script")) {
      ha_track_text(entity_id, "mode", this->mode_[idx]);
    }
  }

  const std::string &last_triggered(int idx) const { return at_(this->last_triggered_, idx); }
  const std::string &mode(int idx) const { return at_(this->mode_, idx); }

 protected:
  std::array<std::string, AUTOMATION_LIST_COUNT> last_triggered_{};
  std::array<std::string, AUTOMATION_LIST_COUNT> mode_{};
};

class AlarmStatusTracker : public SingleStateTracker<ALARM_LIST_COUNT> {
 public:
  AlarmStatusTracker() : SingleStateTracker(ALARM_LIST) {}

  // Which arm modes the panel supports, and whether it can be triggered.
  void subscribe(int idx) {
    SingleStateTracker<ALARM_LIST_COUNT>::subscribe(idx);
    ha_track_int(this->entity_id(idx), "supported_features", this->supported_features_[idx]);
  }

  // -1 until Home Assistant has sent them.
  int supported_features(int idx) const { return in_range_(idx) ? this->supported_features_[idx] : -1; }

 protected:
  std::array<int, ALARM_LIST_COUNT> supported_features_ = filled_array<int, ALARM_LIST_COUNT>(-1);
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

class WeatherStatusTracker : public EntityTracker<WEATHER_LIST_COUNT> {
 public:
  WeatherStatusTracker() : EntityTracker(WEATHER_LIST) {}

  // The unit attributes come first: every value below is in the unit the
  // weather entity reports, which Home Assistant can set per entity.
  void subscribe(int idx) {
    const char *entity_id = this->entity_id(idx);
    ha_track_state(entity_id, nullptr, this->state_[idx]);
    ha_track_text(entity_id, "temperature_unit", this->temperature_unit_[idx]);
    ha_track_text(entity_id, "wind_speed_unit", this->wind_speed_unit_[idx]);
    ha_track_text(entity_id, "pressure_unit", this->pressure_unit_[idx]);
    ha_track_text(entity_id, "precipitation_unit", this->precipitation_unit_[idx]);
    ha_track_float(entity_id, "temperature", this->temperature_[idx]);
    ha_track_float(entity_id, "humidity", this->humidity_[idx]);
    ha_track_float(entity_id, "wind_speed", this->wind_speed_[idx]);
    // Degrees, or a compass point ("NW") from some integrations.
    float *bearing = &this->wind_bearing_[idx];
    ha_subscribe(entity_id, "wind_bearing", [bearing](esphome::StringRef state) {
      float value = ha_parse_float(state);
      *bearing = std::isnan(value) ? compass_point_degrees(state.c_str(), state.size()) : value;
    });
    ha_track_float(entity_id, "wind_gust_speed", this->wind_gust_speed_[idx]);
    ha_track_float(entity_id, "pressure", this->pressure_[idx]);
    ha_track_float(entity_id, "cloud_coverage", this->cloud_coverage_[idx]);
    ha_track_float(entity_id, "uv_index", this->uv_index_[idx]);
    ha_track_float(entity_id, "dew_point", this->dew_point_[idx]);
    ha_track_float(entity_id, "apparent_temperature", this->apparent_temperature_[idx]);
  }

  const std::string &state(int idx) const { return at_(this->state_, idx, unknown_string()); }
  float temperature(int idx) const { return at_(this->temperature_, idx); }
  float humidity(int idx) const { return at_(this->humidity_, idx); }
  float high_temperature(int idx) const { return at_(this->high_temperature_, idx); }
  float low_temperature(int idx) const { return at_(this->low_temperature_, idx); }
  float wind_speed(int idx) const { return at_(this->wind_speed_, idx); }
  float wind_bearing(int idx) const { return at_(this->wind_bearing_, idx); }
  float wind_gust_speed(int idx) const { return at_(this->wind_gust_speed_, idx); }
  float pressure(int idx) const { return at_(this->pressure_, idx); }
  float cloud_coverage(int idx) const { return at_(this->cloud_coverage_, idx); }
  float uv_index(int idx) const { return at_(this->uv_index_, idx); }
  float dew_point(int idx) const { return at_(this->dew_point_, idx); }
  float apparent_temperature(int idx) const { return at_(this->apparent_temperature_, idx); }
  float precipitation(int idx) const { return at_(this->precipitation_, idx); }
  const std::string &temperature_unit(int idx) const { return at_(this->temperature_unit_, idx); }
  const std::string &wind_speed_unit(int idx) const { return at_(this->wind_speed_unit_, idx); }
  const std::string &pressure_unit(int idx) const { return at_(this->pressure_unit_, idx); }
  const std::string &precipitation_unit(int idx) const { return at_(this->precipitation_unit_, idx); }

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
    const ForecastRequest &request = this->forecast_[idx];
    return !request.done && request.sent < FORECAST_MAX_REQUESTS &&
           (request.sent_at == 0 || now - request.sent_at >= FORECAST_ANSWER_WAIT_MS);
  }
  const char *forecast_type(int idx) const {
    return in_range_(idx) && this->forecast_[idx].twice_daily ? "twice_daily" : "daily";
  }
  void mark_forecast_sent(int idx, uint32_t now) {
    if (in_range_(idx)) {
      this->forecast_[idx].sent_at = now | 1;  // 0 means "not sent"
      this->forecast_[idx].sent++;
    }
  }
  // Home Assistant refused the request: one for daily forecasts from an
  // integration without them is asked again for twice-daily ones.
  void forecast_failed(int idx) {
    if (!in_range_(idx)) {
      return;
    }
    ForecastRequest &request = this->forecast_[idx];
    if (request.twice_daily) {
      request.done = true;
    } else {
      request.twice_daily = true;
      request.sent_at = 0;
    }
  }
  void store_forecast(int idx, float high, float low, float precipitation) {
    if (in_range_(idx)) {
      this->high_temperature_[idx] = high;
      this->low_temperature_[idx] = low;
      this->precipitation_[idx] = precipitation;
      this->forecast_[idx].done = true;
    }
  }

 protected:
  struct ForecastRequest {
    uint32_t sent_at = 0;
    uint8_t sent = 0;
    bool twice_daily = false;
    bool done = false;
  };

  std::array<ForecastRequest, WEATHER_LIST_COUNT> forecast_{};
  std::array<std::string, WEATHER_LIST_COUNT> state_{};
  std::array<std::string, WEATHER_LIST_COUNT> temperature_unit_{};
  std::array<std::string, WEATHER_LIST_COUNT> wind_speed_unit_{};
  std::array<std::string, WEATHER_LIST_COUNT> pressure_unit_{};
  std::array<std::string, WEATHER_LIST_COUNT> precipitation_unit_{};
  std::array<float, WEATHER_LIST_COUNT> temperature_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> humidity_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> high_temperature_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> low_temperature_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> wind_speed_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> wind_bearing_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> wind_gust_speed_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> pressure_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> cloud_coverage_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> uv_index_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> dew_point_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> apparent_temperature_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> precipitation_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
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
