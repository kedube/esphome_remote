#pragma once

#include <cmath>
#include <cstdlib>
#include <string>

#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/json/json_util.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/string_ref.h"

enum RemoteMode {
  REMOTE_MODE_LIGHTS = 0,
  REMOTE_MODE_CLIMATE = 1,
  REMOTE_MODE_MUSIC = 2,
  REMOTE_MODE_AUTOMATION = 3,
  REMOTE_MODE_WEATHER = 4,
  REMOTE_MODE_TIME = 5,
};

static constexpr int REMOTE_MODE_COUNT = 6;

struct LightEntity {
  const char *name;
  const char *entity_id;
};

struct ClimateEntity {
  const char *name;
  const char *entity_id;
};

struct MediaEntity {
  const char *name;
  const char *entity_id;
};

struct AutomationEntity {
  const char *name;
  const char *entity_id;
};

struct WeatherEntity {
  const char *name;
  const char *entity_id;
};

static const LightEntity LIGHT_LIST[] = {
    {"Office Light", "light.office_light"},
    {"Office Recessed", "light.office_recessed"},
    {"Loft Recessed", "light.loft_recessed"},
    {"Stairway Light", "light.stairway_light"},
    {"Kitchen Recessed", "light.kitchen_recessed"},
    {"Kitchen Island", "light.kitchen_island_pendant_lights"},
    {"Dining Light", "light.dining_room_light"},
    {"Dining Recessed", "light.dining_room_recessed"},
    {"Den Light", "light.den_light"},
    {"Den Recessed", "light.den_recessed"},
    {"Living Room", "light.living_room_recessed"},
    {"Master Bedroom", "light.master_bedroom_recessed"},
    {"Master Bathroom", "light.master_bathroom_recessed"},
    {"F Guest Light", "light.front_guest_bedroom_light"},
    {"F Guest Recessed", "light.front_guest_bedroom_recessed"},
    {"R Guest Light", "light.rear_guest_bedroom_light"},
    {"R Guest Recessed", "light.rear_guest_bedroom_recessed"},
    {"Guest Bathroom", "light.guest_bathroom_recessed"},
    {"Bathroom", "light.bathroom_recessed"},
    {"Garage Lights", "light.garage_lights"},
    {"Upstairs Hallway", "light.upstairs_hallway"},
    {"Mudroom Hallway", "light.mudroom_hallway_recessed"},
    {"Porch Recessed", "light.porch_recessed"},
    {"Patio", "light.patio_flood_lights"},
};

static const ClimateEntity CLIMATE_LIST[] = {
    {"Living Room", "climate.living_room_thermostat"},
    {"Den", "climate.den_thermostat"},
    {"Entryway", "climate.entryway_thermostat"},
    {"Master Bedroom", "climate.master_bedroom_thermostat"},
    {"Master Bathroom", "climate.bathroom_thermostat"},
    {"2nd Floor Hallway", "climate.hallway_thermostat"},
    {"Garage", "climate.garage_thermostat"},
};

static const MediaEntity MEDIA_PLAYER_LIST[] = {
    {"Office Sonos", "media_player.office_sonos"},
    {"Office ERA300", "media_player.office"},
    {"Kitchen Sonos", "media_player.kitchen_sonos"},
    {"Loft TV", "media_player.samsung_s95ba_55_tv"},
    {"Den TV", "media_player.samsung_s95bd_55_tv"},
  };

static const AutomationEntity AUTOMATION_LIST[] = {
    {"Christmas Lights", "script.christmas_lights"},
    {"Exterior Lights On", "script.turn_on_exterior_lights"},
    {"Exterior Lights Off", "script.turn_off_exterior_lights"},
};

static const WeatherEntity WEATHER_LIST[] = {
    {"PirateWeather", "weather.pirateweather"},
    {"WeatherFlow", "weather.weatherflow_forecast"},
};

static const int LIGHT_LIST_COUNT = sizeof(LIGHT_LIST) / sizeof(LIGHT_LIST[0]);
static const int CLIMATE_LIST_COUNT = sizeof(CLIMATE_LIST) / sizeof(CLIMATE_LIST[0]);
static const int MEDIA_PLAYER_LIST_COUNT = sizeof(MEDIA_PLAYER_LIST) / sizeof(MEDIA_PLAYER_LIST[0]);
static const int AUTOMATION_LIST_COUNT = sizeof(AUTOMATION_LIST) / sizeof(AUTOMATION_LIST[0]);
static const int WEATHER_LIST_COUNT = sizeof(WEATHER_LIST) / sizeof(WEATHER_LIST[0]);

static inline bool ha_state_missing(const std::string &value) {
  return value.empty() || value == "unknown" || value == "unavailable" || value == "None";
}

static inline std::string ha_state_or_unknown(esphome::StringRef state) {
  std::string value = state.str();
  return ha_state_missing(value) ? "unknown" : value;
}

static inline float ha_parse_float(esphome::StringRef state) {
  std::string value = state.str();
  return ha_state_missing(value) ? NAN : strtof(value.c_str(), nullptr);
}

template <typename Entity>
static inline int find_entity_index(const Entity *entities, int count, const std::string &entity_id) {
  for (int i = 0; i < count; i++) {
    if (entity_id == entities[i].entity_id) {
      return i;
    }
  }
  return -1;
}

static inline const char *mode_title(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return "LIGHTS";
    case REMOTE_MODE_CLIMATE:
      return "CLIMATE";
    case REMOTE_MODE_MUSIC:
      return "MUSIC";
    case REMOTE_MODE_AUTOMATION:
      return "AUTOMATIONS";
    case REMOTE_MODE_WEATHER:
      return "WEATHER";
    case REMOTE_MODE_TIME:
      return "TIME";
    default:
      return "MODE";
  }
}

static inline int mode_item_count(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return LIGHT_LIST_COUNT;
    case REMOTE_MODE_CLIMATE:
      return CLIMATE_LIST_COUNT;
    case REMOTE_MODE_MUSIC:
      return MEDIA_PLAYER_LIST_COUNT;
    case REMOTE_MODE_AUTOMATION:
      return AUTOMATION_LIST_COUNT;
    case REMOTE_MODE_WEATHER:
      return WEATHER_LIST_COUNT;
    case REMOTE_MODE_TIME:
      return 1;
    default:
      return 0;
  }
}

static inline bool mode_is_available(RemoteMode mode) {
  return mode_item_count(mode) > 0;
}

static inline RemoteMode first_available_mode() {
  for (int i = 0; i < REMOTE_MODE_COUNT; i++) {
    RemoteMode mode = static_cast<RemoteMode>(i);
    if (mode_is_available(mode)) {
      return mode;
    }
  }
  return REMOTE_MODE_TIME;
}

static inline RemoteMode next_available_mode(RemoteMode current, int step) {
  if (step == 0) {
    return mode_is_available(current) ? current : first_available_mode();
  }

  for (int i = 0; i < REMOTE_MODE_COUNT; i++) {
    int next = (int(current) + step + (step > 0 ? i : -i) + REMOTE_MODE_COUNT * 2) % REMOTE_MODE_COUNT;
    RemoteMode mode = static_cast<RemoteMode>(next);
    if (mode_is_available(mode)) {
      return mode;
    }
  }
  return first_available_mode();
}

static inline std::string mode_item_name(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return (idx >= 0 && idx < LIGHT_LIST_COUNT) ? LIGHT_LIST[idx].name : "";
    case REMOTE_MODE_CLIMATE:
      return (idx >= 0 && idx < CLIMATE_LIST_COUNT) ? CLIMATE_LIST[idx].name : "";
    case REMOTE_MODE_MUSIC:
      return (idx >= 0 && idx < MEDIA_PLAYER_LIST_COUNT) ? MEDIA_PLAYER_LIST[idx].name : "";
    case REMOTE_MODE_AUTOMATION:
      return (idx >= 0 && idx < AUTOMATION_LIST_COUNT) ? AUTOMATION_LIST[idx].name : "";
    case REMOTE_MODE_WEATHER:
      return (idx >= 0 && idx < WEATHER_LIST_COUNT) ? WEATHER_LIST[idx].name : "";
    case REMOTE_MODE_TIME:
      return "";
    default:
      return "";
  }
}

static inline std::string mode_item_entity(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return (idx >= 0 && idx < LIGHT_LIST_COUNT) ? LIGHT_LIST[idx].entity_id : "";
    case REMOTE_MODE_CLIMATE:
      return (idx >= 0 && idx < CLIMATE_LIST_COUNT) ? CLIMATE_LIST[idx].entity_id : "";
    case REMOTE_MODE_MUSIC:
      return (idx >= 0 && idx < MEDIA_PLAYER_LIST_COUNT) ? MEDIA_PLAYER_LIST[idx].entity_id : "";
    case REMOTE_MODE_AUTOMATION:
      return (idx >= 0 && idx < AUTOMATION_LIST_COUNT) ? AUTOMATION_LIST[idx].entity_id : "";
    case REMOTE_MODE_WEATHER:
      return (idx >= 0 && idx < WEATHER_LIST_COUNT) ? WEATHER_LIST[idx].entity_id : "";
    case REMOTE_MODE_TIME:
      return "";
    default:
      return "";
  }
}

enum AutomationKind {
  AUTOMATION_KIND_AUTOMATION = 0,
  AUTOMATION_KIND_SCRIPT = 1,
  AUTOMATION_KIND_SCENE = 2,
};

static inline AutomationKind automation_kind(int idx) {
  if (idx < 0 || idx >= AUTOMATION_LIST_COUNT) {
    return AUTOMATION_KIND_SCRIPT;
  }

  std::string entity_id = AUTOMATION_LIST[idx].entity_id;
  size_t separator = entity_id.find('.');
  std::string domain = separator == std::string::npos ? entity_id : entity_id.substr(0, separator);

  if (domain == "automation") {
    return AUTOMATION_KIND_AUTOMATION;
  }
  if (domain == "scene") {
    return AUTOMATION_KIND_SCENE;
  }
  return AUTOMATION_KIND_SCRIPT;
}

static inline bool automation_supports_enabled_state(int idx) {
  return automation_kind(idx) == AUTOMATION_KIND_AUTOMATION;
}

static inline const char *automation_kind_label(int idx) {
  switch (automation_kind(idx)) {
    case AUTOMATION_KIND_AUTOMATION:
      return "AUTOMATION";
    case AUTOMATION_KIND_SCENE:
      return "SCENE";
    case AUTOMATION_KIND_SCRIPT:
    default:
      return "SCRIPT";
  }
}

static inline int &selected_index_ref(
    RemoteMode mode, int &light_idx, int &climate_idx, int &media_idx, int &automation_idx, int &weather_idx,
    int &time_idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return light_idx;
    case REMOTE_MODE_CLIMATE:
      return climate_idx;
    case REMOTE_MODE_MUSIC:
      return media_idx;
    case REMOTE_MODE_AUTOMATION:
      return automation_idx;
    case REMOTE_MODE_WEATHER:
      return weather_idx;
    case REMOTE_MODE_TIME:
    default:
      return time_idx;
  }
}

static inline int clamp_mode_index(int idx, int count) {
  if (count <= 0) {
    return 0;
  }
  return (idx >= 0 && idx < count) ? idx : 0;
}

static inline int wrapped_mode_index(int idx, int count, int step) {
  if (count <= 0) {
    return 0;
  }
  idx = clamp_mode_index(idx, count);
  idx = (idx + (step % count) + count) % count;
  return idx;
}

static inline const std::string &unknown_string() {
  static const std::string unknown = "unknown";
  return unknown;
}

static inline const std::string &empty_string() {
  static const std::string empty;
  return empty;
}

class LightStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < LIGHT_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&LightStatusTracker::on_light_state_, LIGHT_LIST[i].entity_id);
      this->subscribe_homeassistant_state(
          &LightStatusTracker::on_light_brightness_, LIGHT_LIST[i].entity_id, "brightness");
    }
    this->request_all_states();
  }

  void request_light_state(int idx) {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return;
    }

    const char *entity_id = LIGHT_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "brightness", [this, idx](esphome::StringRef state) { this->store_brightness_(idx, state); });
  }

  void request_all_states() {
    for (int i = 0; i < LIGHT_LIST_COUNT; i++) {
      this->request_light_state(i);
    }
  }

  bool has_state(int idx) const { return idx >= 0 && idx < LIGHT_LIST_COUNT && this->has_state_[idx]; }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_brightness(int idx) const { return idx >= 0 && idx < LIGHT_LIST_COUNT && this->has_brightness_[idx]; }

  float brightness(int idx) const {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return NAN;
    }
    return this->brightness_[idx];
  }

 protected:
  void on_light_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_light_brightness_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_brightness_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
    this->has_state_[idx] = this->state_[idx] != "unknown";
  }

  void store_brightness_(int idx, esphome::StringRef state) {
    this->brightness_[idx] = ha_parse_float(state);
    this->has_brightness_[idx] = !std::isnan(this->brightness_[idx]);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(LIGHT_LIST, LIGHT_LIST_COUNT, entity_id);
  }

  std::string state_[LIGHT_LIST_COUNT];
  float brightness_[LIGHT_LIST_COUNT] = {NAN};
  bool has_state_[LIGHT_LIST_COUNT] = {false};
  bool has_brightness_[LIGHT_LIST_COUNT] = {false};
};

class ClimateStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < CLIMATE_LIST_COUNT; i++) {
      const char *entity_id = CLIMATE_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_temperature_, entity_id, "temperature");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_temperature_low_, entity_id, "target_temp_low");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_temperature_high_, entity_id, "target_temp_high");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_current_temperature_, entity_id, "current_temperature");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_hvac_action_, entity_id, "hvac_action");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_preset_mode_, entity_id, "preset_mode");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_preset_modes_, entity_id, "preset_modes");
    }
    this->request_all_states();
  }

  void request_climate_state(int idx) {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return;
    }

    const char *entity_id = CLIMATE_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "temperature",
        [this, idx](esphome::StringRef state) { this->store_target_temperature_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "target_temp_low",
        [this, idx](esphome::StringRef state) { this->store_target_temperature_low_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "target_temp_high",
        [this, idx](esphome::StringRef state) { this->store_target_temperature_high_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "current_temperature",
        [this, idx](esphome::StringRef state) { this->store_current_temperature_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "hvac_action",
        [this, idx](esphome::StringRef state) { this->store_hvac_action_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "preset_mode", [this, idx](esphome::StringRef state) { this->store_preset_mode_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "preset_modes", [this, idx](esphome::StringRef state) { this->store_preset_modes_(idx, state); });
  }

  void request_all_states() {
    for (int i = 0; i < CLIMATE_LIST_COUNT; i++) {
      this->request_climate_state(i);
    }
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  const std::string &hvac_action(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return unknown_string();
    }
    return this->hvac_action_[idx];
  }

  const std::string &preset_mode(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->preset_mode_[idx];
  }

  bool supports_preset(int idx) const {
    return idx >= 0 && idx < CLIMATE_LIST_COUNT && this->supports_preset_[idx];
  }

  float target_temperature(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_temperature_[idx];
  }

  float current_temperature(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->current_temperature_[idx];
  }

  float target_temperature_low(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_temperature_low_[idx];
  }

  float target_temperature_high(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_temperature_high_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_target_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_temperature_(idx, state);
    }
  }

  void on_current_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_current_temperature_(idx, state);
    }
  }

  void on_target_temperature_low_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_temperature_low_(idx, state);
    }
  }

  void on_target_temperature_high_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_temperature_high_(idx, state);
    }
  }

  void on_hvac_action_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_hvac_action_(idx, state);
    }
  }

  void on_preset_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_preset_mode_(idx, state);
    }
  }

  void on_preset_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_preset_modes_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_target_temperature_(int idx, esphome::StringRef state) {
    this->target_temperature_[idx] = ha_parse_float(state);
  }

  void store_target_temperature_low_(int idx, esphome::StringRef state) {
    this->target_temperature_low_[idx] = ha_parse_float(state);
  }

  void store_target_temperature_high_(int idx, esphome::StringRef state) {
    this->target_temperature_high_[idx] = ha_parse_float(state);
  }

  void store_current_temperature_(int idx, esphome::StringRef state) {
    this->current_temperature_[idx] = ha_parse_float(state);
  }

  void store_hvac_action_(int idx, esphome::StringRef state) {
    this->hvac_action_[idx] = ha_state_or_unknown(state);
  }

  void store_preset_mode_(int idx, esphome::StringRef state) { this->preset_mode_[idx] = state.str(); }

  void store_preset_modes_(int idx, esphome::StringRef state) {
    this->supports_preset_[idx] = false;

    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    if (doc.is<JsonArray>()) {
      JsonArray preset_modes = doc.as<JsonArray>();
      this->supports_preset_[idx] = !preset_modes.isNull() && preset_modes.size() > 0;
      return;
    }

    this->supports_preset_[idx] = payload != "[]" && !payload.empty();
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(CLIMATE_LIST, CLIMATE_LIST_COUNT, entity_id);
  }

  std::string state_[CLIMATE_LIST_COUNT];
  std::string hvac_action_[CLIMATE_LIST_COUNT];
  std::string preset_mode_[CLIMATE_LIST_COUNT];
  float target_temperature_[CLIMATE_LIST_COUNT] = {NAN};
  float target_temperature_low_[CLIMATE_LIST_COUNT] = {NAN};
  float target_temperature_high_[CLIMATE_LIST_COUNT] = {NAN};
  float current_temperature_[CLIMATE_LIST_COUNT] = {NAN};
  bool supports_preset_[CLIMATE_LIST_COUNT] = {false};
};

class MediaStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < MEDIA_PLAYER_LIST_COUNT; i++) {
      const char *entity_id = MEDIA_PLAYER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_volume_, entity_id, "volume_level");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_title_, entity_id, "media_title");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_artist_, entity_id, "media_artist");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_source_, entity_id, "source");
    }
    this->request_all_states();
  }

  void request_media_state(int idx) {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return;
    }

    const char *entity_id = MEDIA_PLAYER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "volume_level", [this, idx](esphome::StringRef state) { this->store_volume_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "media_title", [this, idx](esphome::StringRef state) { this->store_title_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "media_artist", [this, idx](esphome::StringRef state) { this->store_artist_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "source", [this, idx](esphome::StringRef state) { this->store_source_(idx, state); });
  }

  void request_all_states() {
    for (int i = 0; i < MEDIA_PLAYER_LIST_COUNT; i++) {
      this->request_media_state(i);
    }
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  const std::string &title(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->title_[idx];
  }

  const std::string &artist(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->artist_[idx];
  }

  const std::string &source(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->source_[idx];
  }

  float volume(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return NAN;
    }
    return this->volume_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_volume_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_volume_(idx, state);
    }
  }

  void on_title_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_title_(idx, state);
    }
  }

  void on_artist_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_artist_(idx, state);
    }
  }

  void on_source_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_source_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_title_(int idx, esphome::StringRef state) { this->title_[idx] = state.str(); }

  void store_artist_(int idx, esphome::StringRef state) { this->artist_[idx] = state.str(); }

  void store_source_(int idx, esphome::StringRef state) { this->source_[idx] = state.str(); }

  void store_volume_(int idx, esphome::StringRef state) {
    this->volume_[idx] = ha_parse_float(state);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, entity_id);
  }

  std::string state_[MEDIA_PLAYER_LIST_COUNT];
  std::string title_[MEDIA_PLAYER_LIST_COUNT];
  std::string artist_[MEDIA_PLAYER_LIST_COUNT];
  std::string source_[MEDIA_PLAYER_LIST_COUNT];
  float volume_[MEDIA_PLAYER_LIST_COUNT] = {NAN};
};

class AutomationStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < AUTOMATION_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&AutomationStatusTracker::on_state_, AUTOMATION_LIST[i].entity_id);
    }
    this->request_all_states();
  }

  void request_automation_state(int idx) {
    if (idx < 0 || idx >= AUTOMATION_LIST_COUNT) {
      return;
    }

    const char *entity_id = AUTOMATION_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
  }

  void request_all_states() {
    for (int i = 0; i < AUTOMATION_LIST_COUNT; i++) {
      this->request_automation_state(i);
    }
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= AUTOMATION_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(AUTOMATION_LIST, AUTOMATION_LIST_COUNT, entity_id);
  }

  std::string state_[AUTOMATION_LIST_COUNT];
};

class WeatherStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < WEATHER_LIST_COUNT; i++) {
      const char *entity_id = WEATHER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_temperature_, entity_id, "temperature");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_humidity_, entity_id, "humidity");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_forecast_, entity_id, "forecast");
    }
    this->request_all_states();
  }

  void request_weather_state(int idx) {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return;
    }

    const char *entity_id = WEATHER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "temperature",
        [this, idx](esphome::StringRef state) { this->store_temperature_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "humidity", [this, idx](esphome::StringRef state) { this->store_humidity_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "forecast", [this, idx](esphome::StringRef state) { this->store_forecast_(idx, state); });
  }

  void request_all_states() {
    for (int i = 0; i < WEATHER_LIST_COUNT; i++) {
      this->request_weather_state(i);
    }
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  float temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->temperature_[idx];
  }

  float humidity(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->humidity_[idx];
  }

  float high_temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->high_temperature_[idx];
  }

  float low_temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->low_temperature_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_temperature_(idx, state);
    }
  }

  void on_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_humidity_(idx, state);
    }
  }

  void on_forecast_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_forecast_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_temperature_(int idx, esphome::StringRef state) {
    this->temperature_[idx] = ha_parse_float(state);
  }

  void store_humidity_(int idx, esphome::StringRef state) {
    this->humidity_[idx] = ha_parse_float(state);
  }

  void store_forecast_(int idx, esphome::StringRef state) {
    this->high_temperature_[idx] = NAN;
    this->low_temperature_[idx] = NAN;

    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    JsonArray forecast;
    if (doc.is<JsonArray>()) {
      forecast = doc.as<JsonArray>();
    } else if (doc.is<JsonObject>()) {
      JsonObject root = doc.as<JsonObject>();
      forecast = root["forecast"].as<JsonArray>();
    } else {
      return;
    }

    if (forecast.isNull() || forecast.size() == 0) {
      return;
    }

    JsonObject today = forecast[0].as<JsonObject>();
    if (today.isNull()) {
      return;
    }

    auto read_number = [&](JsonObject obj, const char *key) -> float {
      if (obj[key].is<float>() || obj[key].is<int>()) {
        return obj[key].as<float>();
      }
      if (obj[key].is<const char *>()) {
        const char *value = obj[key].as<const char *>();
        if (value != nullptr && value[0] != '\0') {
          return strtof(value, nullptr);
        }
      }
      return NAN;
    };

    float high = read_number(today, "temperature");
    if (std::isnan(high)) high = read_number(today, "temperature_high");
    if (std::isnan(high)) high = read_number(today, "native_temperature");
    if (std::isnan(high)) high = read_number(today, "native_temperature_high");

    float low = read_number(today, "templow");
    if (std::isnan(low)) low = read_number(today, "temperature_low");
    if (std::isnan(low)) low = read_number(today, "native_templow");
    if (std::isnan(low)) low = read_number(today, "native_temperature_low");

    this->high_temperature_[idx] = high;
    this->low_temperature_[idx] = low;
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(WEATHER_LIST, WEATHER_LIST_COUNT, entity_id);
  }

  std::string state_[WEATHER_LIST_COUNT];
  float temperature_[WEATHER_LIST_COUNT] = {NAN};
  float humidity_[WEATHER_LIST_COUNT] = {NAN};
  float high_temperature_[WEATHER_LIST_COUNT] = {NAN};
  float low_temperature_[WEATHER_LIST_COUNT] = {NAN};
};

static LightStatusTracker light_status_tracker_storage;
static ClimateStatusTracker climate_status_tracker_storage;
static MediaStatusTracker media_status_tracker_storage;
static AutomationStatusTracker automation_status_tracker_storage;
static WeatherStatusTracker weather_status_tracker_storage;
static bool remote_status_trackers_initialized = false;

static inline void ensure_remote_status_trackers() {
  if (remote_status_trackers_initialized) {
    return;
  }
  light_status_tracker_storage.setup();
  climate_status_tracker_storage.setup();
  media_status_tracker_storage.setup();
  automation_status_tracker_storage.setup();
  weather_status_tracker_storage.setup();
  remote_status_trackers_initialized = true;
}

static inline void ensure_light_status_tracker() { ensure_remote_status_trackers(); }

static inline void request_selected_light_status(int idx) {
  ensure_remote_status_trackers();
  light_status_tracker_storage.request_light_state(idx);
}

static inline bool selected_light_has_state(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.has_state(idx);
}

static inline const std::string &selected_light_state(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.state(idx);
}

static inline bool selected_light_has_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.has_brightness(idx);
}

static inline float selected_light_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.brightness(idx);
}

static inline void request_selected_climate_status(int idx) {
  ensure_remote_status_trackers();
  climate_status_tracker_storage.request_climate_state(idx);
}

static inline const std::string &selected_climate_state(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.state(idx);
}

static inline const std::string &climate_hvac_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.hvac_action(idx);
}

static inline const std::string &selected_climate_preset_mode(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.preset_mode(idx);
}

static inline bool climate_supports_preset(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.supports_preset(idx);
}

static inline float selected_climate_target_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature(idx);
}

static inline float selected_climate_target_temperature_low(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_low(idx);
}

static inline float selected_climate_target_temperature_high(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_high(idx);
}

static inline float selected_climate_current_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.current_temperature(idx);
}

static inline void request_selected_media_status(int idx) {
  ensure_remote_status_trackers();
  media_status_tracker_storage.request_media_state(idx);
}

static inline const std::string &selected_media_state(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.state(idx);
}

static inline const std::string &media_title_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.title(idx);
}

static inline const std::string &media_artist_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.artist(idx);
}

static inline const std::string &media_source_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source(idx);
}

static inline float selected_media_volume(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.volume(idx);
}

static inline void request_selected_automation_status(int idx) {
  ensure_remote_status_trackers();
  automation_status_tracker_storage.request_automation_state(idx);
}

static inline const std::string &automation_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.state(idx);
}

static inline void request_selected_weather_status(int idx) {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.request_weather_state(idx);
}

static inline void request_all_weather_status() {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.request_all_states();
}

static inline const std::string &weather_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.state(idx);
}

static inline float weather_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.temperature(idx);
}

static inline float weather_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.humidity(idx);
}

static inline float weather_high_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.high_temperature(idx);
}

static inline float weather_low_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.low_temperature(idx);
}

static inline void request_mode_status(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      request_selected_light_status(idx);
      break;
    case REMOTE_MODE_CLIMATE:
      request_selected_climate_status(idx);
      break;
    case REMOTE_MODE_MUSIC:
      request_selected_media_status(idx);
      break;
    case REMOTE_MODE_AUTOMATION:
      request_selected_automation_status(idx);
      break;
    case REMOTE_MODE_WEATHER:
      request_selected_weather_status(idx);
      break;
    case REMOTE_MODE_TIME:
      break;
  }
}
