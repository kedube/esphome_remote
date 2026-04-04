#pragma once

#include "entity_helpers_common.h"

template <typename Entity, int Count>
class SingleStateTracker : public esphome::api::CustomAPIDevice {
 public:
  explicit SingleStateTracker(const Entity *entities) : entities_(entities) {}

  void setup() {
    for (int i = 0; i < Count; i++) {
      this->subscribe_homeassistant_state(&SingleStateTracker::on_state_, this->entities_[i].entity_id);
    }
    this->request_all_states();
  }

  void request_state(int idx) {
    if (idx < 0 || idx >= Count) {
      return;
    }

    const char *entity_id = this->entities_[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
  }

  void request_all_states() { request_all_states_by_count(Count, [this](int i) { this->request_state(i); }); }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= Count) {
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

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(this->entities_, Count, entity_id);
  }

  const Entity *entities_;
  std::array<std::string, Count> state_{};
};

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
    request_all_states_by_count(LIGHT_LIST_COUNT, [this](int i) { this->request_light_state(i); });
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

  std::array<std::string, LIGHT_LIST_COUNT> state_{};
  std::array<float, LIGHT_LIST_COUNT> brightness_ = filled_array<float, LIGHT_LIST_COUNT>(NAN);
  std::array<bool, LIGHT_LIST_COUNT> has_state_{};
  std::array<bool, LIGHT_LIST_COUNT> has_brightness_{};
};

using SwitchStatusTracker = SingleStateTracker<SwitchEntity, SWITCH_LIST_COUNT>;

class FanStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < FAN_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_state_, FAN_LIST[i].entity_id);
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_percentage_, FAN_LIST[i].entity_id, "percentage");
    }
    this->request_all_states();
  }

  void request_fan_state(int idx) {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return;
    }

    const char *entity_id = FAN_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "percentage", [this, idx](esphome::StringRef state) { this->store_percentage_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(FAN_LIST_COUNT, [this](int i) { this->request_fan_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_percentage(int idx) const { return idx >= 0 && idx < FAN_LIST_COUNT && this->has_percentage_[idx]; }

  float percentage(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return NAN;
    }
    return this->percentage_[idx];
  }

 protected:
  void on_fan_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_fan_percentage_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_percentage_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }

  void store_percentage_(int idx, esphome::StringRef state) {
    this->percentage_[idx] = ha_parse_float(state);
    this->has_percentage_[idx] = !std::isnan(this->percentage_[idx]);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(FAN_LIST, FAN_LIST_COUNT, entity_id);
  }

  std::array<std::string, FAN_LIST_COUNT> state_{};
  std::array<float, FAN_LIST_COUNT> percentage_ = filled_array<float, FAN_LIST_COUNT>(NAN);
  std::array<bool, FAN_LIST_COUNT> has_percentage_{};
};

class HumidifierStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < HUMIDIFIER_LIST_COUNT; i++) {
      const char *entity_id = HUMIDIFIER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_mode_, entity_id, "mode");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_available_modes_, entity_id, "available_modes");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_target_humidity_, entity_id, "humidity");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_current_humidity_, entity_id, "current_humidity");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_action_, entity_id, "action");
    }
    this->request_all_states();
  }

  void request_humidifier_state(int idx) {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return;
    }

    const char *entity_id = HUMIDIFIER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "mode", [this, idx](esphome::StringRef state) { this->store_mode_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "available_modes",
        [this, idx](esphome::StringRef state) { this->store_available_modes_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "humidity",
        [this, idx](esphome::StringRef state) { this->store_target_humidity_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "current_humidity",
        [this, idx](esphome::StringRef state) { this->store_current_humidity_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "action", [this, idx](esphome::StringRef state) { this->store_action_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(HUMIDIFIER_LIST_COUNT, [this](int i) { this->request_humidifier_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  const std::string &action(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return unknown_string();
    }
    return this->action_[idx];
  }

  const std::string &mode(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return empty_string();
    }
    return this->mode_[idx];
  }

  const std::string &available_modes(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return empty_string();
    }
    return this->available_modes_[idx];
  }

  float target_humidity(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return NAN;
    }
    return this->target_humidity_[idx];
  }

  float current_humidity(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return NAN;
    }
    return this->current_humidity_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_mode_(idx, state);
    }
  }

  void on_available_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_available_modes_(idx, state);
    }
  }

  void on_target_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_humidity_(idx, state);
    }
  }

  void on_current_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_current_humidity_(idx, state);
    }
  }

  void on_action_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_action_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }
  void store_mode_(int idx, esphome::StringRef state) { this->mode_[idx] = state.str(); }
  void store_available_modes_(int idx, esphome::StringRef state) {
    this->available_modes_[idx].clear();
    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    if (!doc.is<JsonArray>()) {
      return;
    }

    JsonArray mode_list = doc.as<JsonArray>();
    bool first = true;
    for (JsonVariant value : mode_list) {
      std::string mode = trim_copy(value.as<std::string>());
      if (mode.empty()) {
        continue;
      }
      if (!first) {
        this->available_modes_[idx] += "|";
      }
      this->available_modes_[idx] += mode;
      first = false;
    }
  }
  void store_target_humidity_(int idx, esphome::StringRef state) { this->target_humidity_[idx] = ha_parse_float(state); }
  void store_current_humidity_(int idx, esphome::StringRef state) { this->current_humidity_[idx] = ha_parse_float(state); }
  void store_action_(int idx, esphome::StringRef state) { this->action_[idx] = ha_state_or_unknown(state); }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, entity_id);
  }

  std::array<std::string, HUMIDIFIER_LIST_COUNT> state_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> mode_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> available_modes_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> action_{};
  std::array<float, HUMIDIFIER_LIST_COUNT> target_humidity_ =
      filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
  std::array<float, HUMIDIFIER_LIST_COUNT> current_humidity_ =
      filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
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
    request_all_states_by_count(CLIMATE_LIST_COUNT, [this](int i) { this->request_climate_state(i); });
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

  std::array<std::string, CLIMATE_LIST_COUNT> state_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_action_{};
  std::array<std::string, CLIMATE_LIST_COUNT> preset_mode_{};
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_low_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_high_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> current_temperature_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<bool, CLIMATE_LIST_COUNT> supports_preset_{};
};

using LockStatusTracker = SingleStateTracker<LockEntity, LOCK_LIST_COUNT>;
using SensorStatusTracker = SingleStateTracker<SensorEntity, SENSOR_LIST_COUNT>;

class CoverStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < COVER_LIST_COUNT; i++) {
      const char *entity_id = COVER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_position_, entity_id, "current_position");
    }
    this->request_all_states();
  }

  void request_cover_state(int idx) {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return;
    }

    const char *entity_id = COVER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "current_position", [this, idx](esphome::StringRef state) { this->store_position_(idx, state); });
  }

  void request_all_states() { request_all_states_by_count(COVER_LIST_COUNT, [this](int i) { this->request_cover_state(i); }); }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_position(int idx) const { return idx >= 0 && idx < COVER_LIST_COUNT && this->has_position_[idx]; }

  float position(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return NAN;
    }
    return this->position_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_position_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_position_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }

  void store_position_(int idx, esphome::StringRef state) {
    float value = ha_parse_float(state);
    this->position_[idx] = value;
    this->has_position_[idx] = !std::isnan(value);
  }

  int find_index_(const std::string &entity_id) const { return find_entity_index(COVER_LIST, COVER_LIST_COUNT, entity_id); }

  std::array<std::string, COVER_LIST_COUNT> state_{};
  std::array<float, COVER_LIST_COUNT> position_ = filled_array<float, COVER_LIST_COUNT>(NAN);
  std::array<bool, COVER_LIST_COUNT> has_position_{};
};

class MediaStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < MEDIA_PLAYER_LIST_COUNT; i++) {
      const char *entity_id = MEDIA_PLAYER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_device_class_, entity_id, "device_class");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_source_list_, entity_id, "source_list");
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
        entity_id, "device_class", [this, idx](esphome::StringRef state) { this->store_device_class_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "source_list", [this, idx](esphome::StringRef state) { this->store_source_list_(idx, state); });
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
    request_all_states_by_count(MEDIA_PLAYER_LIST_COUNT, [this](int i) { this->request_media_state(i); });
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

  const std::string &device_class(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->device_class_[idx];
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

  void set_source(int idx, const std::string &source) {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return;
    }
    this->source_[idx] = source;
  }

  const std::string &source_list(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->source_list_[idx];
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

  void on_device_class_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_device_class_(idx, state);
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

  void on_source_list_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_source_list_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_title_(int idx, esphome::StringRef state) { this->title_[idx] = state.str(); }

  void store_artist_(int idx, esphome::StringRef state) { this->artist_[idx] = state.str(); }

  void store_source_(int idx, esphome::StringRef state) {
    std::string source = state.str();
    std::string device_class = this->device_class_[idx];
    if ((device_class == "tv" || device_class == "receiver") && trim_copy(source).empty()) {
      return;
    }
    this->source_[idx] = source;
  }

  void store_device_class_(int idx, esphome::StringRef state) { this->device_class_[idx] = state.str(); }

  void store_source_list_(int idx, esphome::StringRef state) {
    this->source_list_[idx].clear();
    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    if (!doc.is<JsonArray>()) {
      return;
    }

    JsonArray source_list = doc.as<JsonArray>();
    bool first = true;
    for (JsonVariant value : source_list) {
      std::string source = trim_copy(value.as<std::string>());
      if (source.empty()) {
        continue;
      }
      if (!first) {
        this->source_list_[idx] += "|";
      }
      this->source_list_[idx] += source;
      first = false;
    }
  }

  void store_volume_(int idx, esphome::StringRef state) {
    this->volume_[idx] = ha_parse_float(state);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, entity_id);
  }

  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> state_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> device_class_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> title_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> artist_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> source_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> source_list_{};
  std::array<float, MEDIA_PLAYER_LIST_COUNT> volume_ = filled_array<float, MEDIA_PLAYER_LIST_COUNT>(NAN);
};

using AutomationStatusTracker = SingleStateTracker<AutomationEntity, AUTOMATION_LIST_COUNT>;
using AlarmStatusTracker = SingleStateTracker<AlarmEntity, ALARM_LIST_COUNT>;

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
    request_all_states_by_count(WEATHER_LIST_COUNT, [this](int i) { this->request_weather_state(i); });
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

  std::array<std::string, WEATHER_LIST_COUNT> state_{};
  std::array<float, WEATHER_LIST_COUNT> temperature_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> humidity_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> high_temperature_ =
      filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> low_temperature_ =
      filled_array<float, WEATHER_LIST_COUNT>(NAN);
};

class NotificationFeedTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    if (notification_feed_entity_()[0] == '\0') {
      return;
    }
    this->subscribe_homeassistant_state(
        &NotificationFeedTracker::on_payload_, notification_feed_entity_(),
        notification_feed_attribute_());
    this->subscribe_homeassistant_state(
        &NotificationFeedTracker::on_ids_payload_, notification_feed_entity_(),
        notification_feed_ids_attribute_());
    this->request_notifications();
  }

  void request_notifications() {
    if (notification_feed_entity_()[0] == '\0') {
      this->clear_();
      return;
    }

    esphome::api::global_api_server->get_home_assistant_state(
        notification_feed_entity_(), notification_feed_attribute_(),
        [this](esphome::StringRef state) { this->on_payload_(state); });
    esphome::api::global_api_server->get_home_assistant_state(
        notification_feed_entity_(), notification_feed_ids_attribute_(),
        [this](esphome::StringRef state) { this->on_ids_payload_(state); });
  }

  int item_count() const { return this->count_ > 0 ? this->count_ : 1; }

  std::string label(int idx) const {
    if (this->count_ <= 0) {
      return ""; // NO ALERTS HEADER
    }
    idx = clamp_mode_index(idx, this->count_);
    return std::to_string(idx + 1) + " OF " + std::to_string(this->count_);
  }

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
  static std::string trim_copy_(const std::string &value) {
    size_t start = 0;
    size_t end = value.size();
    while (start < end && (value[start] == ' ' || value[start] == '\n' || value[start] == '\r' || value[start] == '\t')) {
      start++;
    }
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\n' || value[end - 1] == '\r' || value[end - 1] == '\t')) {
      end--;
    }
    return value.substr(start, end - start);
  }

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

  void clear_() {
    this->count_ = 0;
    for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
      this->messages_[i].clear();
      this->ids_[i].clear();
    }
  }

  void on_payload_(esphome::StringRef state) {
    std::string payload = trim_copy_(state.str());
    this->clear_();
    if (ha_state_missing(payload) || payload == "[]" || payload == "none") {
      return;
    }

    const std::string separator = notification_feed_separator_();
    size_t start = 0;
    while (start <= payload.size() && this->count_ < NOTIFICATION_FEED_MAX_ITEMS) {
      size_t end = payload.find(separator, start);
      std::string item = trim_copy_(payload.substr(start, end == std::string::npos ? std::string::npos : end - start));
      if (!item.empty()) {
        this->messages_[this->count_++] = item;
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + separator.size();
    }
  }

  void on_ids_payload_(esphome::StringRef state) {
    std::string payload = trim_copy_(state.str());
    for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
      this->ids_[i].clear();
    }
    if (ha_state_missing(payload) || payload == "[]" || payload == "none") {
      return;
    }

    const std::string separator = notification_feed_separator_();
    size_t start = 0;
    int idx = 0;
    while (start <= payload.size() && idx < NOTIFICATION_FEED_MAX_ITEMS) {
      size_t end = payload.find(separator, start);
      std::string item = trim_copy_(payload.substr(start, end == std::string::npos ? std::string::npos : end - start));
      if (!item.empty()) {
        this->ids_[idx++] = item;
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + separator.size();
    }
  }

  int count_{0};
  std::string messages_[NOTIFICATION_FEED_MAX_ITEMS];
  std::string ids_[NOTIFICATION_FEED_MAX_ITEMS];
};

