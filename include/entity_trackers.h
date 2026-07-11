#pragma once

#include "entity_helpers_common.h"

// Trackers are subscription-driven: Home Assistant pushes the current value of
// every subscribed state/attribute right after the API handshake and streams
// changes afterwards, so no explicit fetches are needed. Note that
// api::APIServer::get_home_assistant_state() must NOT be used for periodic
// refreshes: each call permanently appends to the server's subscription
// vector (it is never pruned), and entries added after the handshake are never
// announced to Home Assistant — a heap leak that fetches nothing.

template <typename Entity, int Count>
class SingleStateTracker : public esphome::api::CustomAPIDevice {
 public:
  explicit SingleStateTracker(const Entity *entities) : entities_(entities) {}

  void setup() {
    for (int i = 0; i < Count; i++) {
      this->subscribe_homeassistant_state(&SingleStateTracker::on_state_, this->entities_[i].entity_id);
    }
  }

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

  void store_state_(int idx, esphome::StringRef state) { ha_assign_state_or_unknown(this->state_[idx], state); }

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
      this->subscribe_homeassistant_state(&LightStatusTracker::on_light_effect_, LIGHT_LIST[i].entity_id, "effect");
      this->subscribe_homeassistant_state(
          &LightStatusTracker::on_light_effect_list_, LIGHT_LIST[i].entity_id, "effect_list");
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

  const std::string &effect(int idx) const {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return empty_string();
    }
    return this->effect_[idx];
  }

  const std::string &effect_list(int idx) const {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return empty_string();
    }
    return this->effect_list_[idx];
  }

  bool has_effect(int idx) const { return idx >= 0 && idx < LIGHT_LIST_COUNT && !this->effect_list_[idx].empty(); }

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

  void on_light_effect_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_effect_(idx, state);
    }
  }

  void on_light_effect_list_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_effect_list_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    ha_assign_state_or_unknown(this->state_[idx], state);
    this->has_state_[idx] = this->state_[idx] != "unknown";
  }

  void store_brightness_(int idx, esphome::StringRef state) {
    this->brightness_[idx] = ha_parse_float(state);
    this->has_brightness_[idx] = !std::isnan(this->brightness_[idx]);
  }

  void store_effect_(int idx, esphome::StringRef state) { ha_assign(this->effect_[idx], state); }

  void store_effect_list_(int idx, esphome::StringRef state) { ha_store_joined_list(this->effect_list_[idx], state); }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(LIGHT_LIST, LIGHT_LIST_COUNT, entity_id);
  }

  std::array<std::string, LIGHT_LIST_COUNT> state_{};
  std::array<std::string, LIGHT_LIST_COUNT> effect_{};
  std::array<std::string, LIGHT_LIST_COUNT> effect_list_{};
  std::array<float, LIGHT_LIST_COUNT> brightness_ = filled_array<float, LIGHT_LIST_COUNT>(NAN);
  std::array<bool, LIGHT_LIST_COUNT> has_state_{};
  std::array<bool, LIGHT_LIST_COUNT> has_brightness_{};
};

using SwitchStatusTracker = SingleStateTracker<EntityEntry, SWITCH_LIST_COUNT>;

class FanStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < FAN_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_state_, FAN_LIST[i].entity_id);
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_percentage_, FAN_LIST[i].entity_id, "percentage");
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_preset_mode_, FAN_LIST[i].entity_id, "preset_mode");
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_preset_modes_, FAN_LIST[i].entity_id, "preset_modes");
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_oscillating_, FAN_LIST[i].entity_id, "oscillating");
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_direction_, FAN_LIST[i].entity_id, "direction");
    }
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

  const std::string &preset_mode(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return empty_string();
    }
    return this->preset_mode_[idx];
  }

  const std::string &preset_modes(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return empty_string();
    }
    return this->preset_modes_[idx];
  }

  const std::string &oscillating(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return empty_string();
    }
    return this->oscillating_[idx];
  }

  const std::string &direction(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return empty_string();
    }
    return this->direction_[idx];
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

  void on_fan_preset_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_preset_mode_(idx, state);
    }
  }

  void on_fan_preset_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_preset_modes_(idx, state);
    }
  }

  void on_fan_oscillating_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_oscillating_(idx, state);
    }
  }

  void on_fan_direction_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_direction_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { ha_assign_state_or_unknown(this->state_[idx], state); }

  void store_percentage_(int idx, esphome::StringRef state) {
    this->percentage_[idx] = ha_parse_float(state);
    this->has_percentage_[idx] = !std::isnan(this->percentage_[idx]);
  }

  void store_preset_mode_(int idx, esphome::StringRef state) { ha_assign(this->preset_mode_[idx], state); }

  void store_preset_modes_(int idx, esphome::StringRef state) { ha_store_joined_list(this->preset_modes_[idx], state); }

  void store_oscillating_(int idx, esphome::StringRef state) { ha_assign(this->oscillating_[idx], state); }

  void store_direction_(int idx, esphome::StringRef state) { ha_assign(this->direction_[idx], state); }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(FAN_LIST, FAN_LIST_COUNT, entity_id);
  }

  std::array<std::string, FAN_LIST_COUNT> state_{};
  std::array<std::string, FAN_LIST_COUNT> preset_mode_{};
  std::array<std::string, FAN_LIST_COUNT> preset_modes_{};
  std::array<std::string, FAN_LIST_COUNT> oscillating_{};
  std::array<std::string, FAN_LIST_COUNT> direction_{};
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

  void store_state_(int idx, esphome::StringRef state) { ha_assign_state_or_unknown(this->state_[idx], state); }
  void store_mode_(int idx, esphome::StringRef state) { ha_assign(this->mode_[idx], state); }
  void store_available_modes_(int idx, esphome::StringRef state) {
    ha_store_joined_list(this->available_modes_[idx], state);
  }
  void store_target_humidity_(int idx, esphome::StringRef state) { this->target_humidity_[idx] = ha_parse_float(state); }
  void store_current_humidity_(int idx, esphome::StringRef state) { this->current_humidity_[idx] = ha_parse_float(state); }
  void store_action_(int idx, esphome::StringRef state) { ha_assign_state_or_unknown(this->action_[idx], state); }

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
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_hvac_mode_, entity_id, "hvac_mode");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_hvac_modes_, entity_id, "hvac_modes");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_fan_mode_, entity_id, "fan_mode");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_fan_modes_, entity_id, "fan_modes");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_humidity_, entity_id, "humidity");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_preset_mode_, entity_id, "preset_mode");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_preset_modes_, entity_id, "preset_modes");
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

  const std::string &preset_modes(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->preset_modes_[idx];
  }

  const std::string &hvac_mode(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->hvac_mode_[idx];
  }

  const std::string &hvac_modes(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->hvac_modes_[idx];
  }

  const std::string &fan_mode(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->fan_mode_[idx];
  }

  const std::string &fan_modes(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->fan_modes_[idx];
  }

  float target_humidity(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_humidity_[idx];
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

  void on_hvac_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_hvac_mode_(idx, state);
  }

  void on_hvac_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_hvac_modes_(idx, state);
  }

  void on_fan_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_fan_mode_(idx, state);
  }

  void on_fan_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_fan_modes_(idx, state);
  }

  void on_target_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_target_humidity_(idx, state);
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
    ha_assign_state_or_unknown(this->state_[idx], state);
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
    std::string &value = this->hvac_action_[idx];
    ha_assign_state_or_unknown(value, state);
    if (value != "unknown") {
      for (auto &c : value) {
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
      }
    }
  }

  void store_hvac_mode_(int idx, esphome::StringRef state) { ha_assign(this->hvac_mode_[idx], state); }

  void store_hvac_modes_(int idx, esphome::StringRef state) { ha_store_joined_list(this->hvac_modes_[idx], state); }

  void store_fan_mode_(int idx, esphome::StringRef state) { ha_assign(this->fan_mode_[idx], state); }

  void store_fan_modes_(int idx, esphome::StringRef state) { ha_store_joined_list(this->fan_modes_[idx], state); }

  void store_target_humidity_(int idx, esphome::StringRef state) { this->target_humidity_[idx] = ha_parse_float(state); }

  void store_preset_mode_(int idx, esphome::StringRef state) { ha_assign(this->preset_mode_[idx], state); }

  void store_preset_modes_(int idx, esphome::StringRef state) {
    ha_store_joined_list(this->preset_modes_[idx], state);
    this->supports_preset_[idx] = !this->preset_modes_[idx].empty();
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(CLIMATE_LIST, CLIMATE_LIST_COUNT, entity_id);
  }

  std::array<std::string, CLIMATE_LIST_COUNT> state_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_action_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_mode_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_modes_{};
  std::array<std::string, CLIMATE_LIST_COUNT> fan_mode_{};
  std::array<std::string, CLIMATE_LIST_COUNT> fan_modes_{};
  std::array<std::string, CLIMATE_LIST_COUNT> preset_mode_{};
  std::array<std::string, CLIMATE_LIST_COUNT> preset_modes_{};
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_low_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_high_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> current_temperature_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_humidity_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<bool, CLIMATE_LIST_COUNT> supports_preset_{};
};

using LockStatusTracker = SingleStateTracker<EntityEntry, LOCK_LIST_COUNT>;

class SensorStatusTracker : public SingleStateTracker<EntityEntry, SENSOR_LIST_COUNT> {
 public:
  using SingleStateTracker<EntityEntry, SENSOR_LIST_COUNT>::SingleStateTracker;

  void setup() {
    for (int i = 0; i < SENSOR_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&SensorStatusTracker::on_state_, this->entities_[i].entity_id);
      this->subscribe_homeassistant_state(
          &SensorStatusTracker::on_unit_, this->entities_[i].entity_id, "suggested_unit_of_measurement");
      this->subscribe_homeassistant_state(
          &SensorStatusTracker::on_unit_, this->entities_[i].entity_id, "unit_of_measurement");
    }
  }

  const std::string &unit(int idx) const {
    if (idx < 0 || idx >= SENSOR_LIST_COUNT) return empty_string();
    return this->unit_[idx];
  }

 protected:
  void on_unit_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_unit_(idx, state);
  }

  void store_unit_(int idx, esphome::StringRef state) {
    std::string unit = trim_copy(state.str());
    if (!unit.empty() && unit != "unknown" && unit != "unavailable") {
      this->unit_[idx] = unit;
    }
  }

  std::array<std::string, SENSOR_LIST_COUNT> unit_{};
};

class CoverStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < COVER_LIST_COUNT; i++) {
      const char *entity_id = COVER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_position_, entity_id, "current_position");
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_tilt_, entity_id, "current_tilt_position");
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_supported_features_, entity_id, "supported_features");
    }
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_position(int idx) const { return idx >= 0 && idx < COVER_LIST_COUNT && (this->supported_features_[idx] & 4) != 0; }

  float position(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return NAN;
    }
    return this->position_[idx];
  }

  bool has_tilt(int idx) const { return idx >= 0 && idx < COVER_LIST_COUNT && (this->supported_features_[idx] & 128) != 0; }

  float tilt(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) return NAN;
    return this->tilt_[idx];
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

  void on_tilt_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_tilt_(idx, state);
  }

  void on_supported_features_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_supported_features_(idx, state);
  }

  void store_state_(int idx, esphome::StringRef state) { ha_assign_state_or_unknown(this->state_[idx], state); }

  void store_position_(int idx, esphome::StringRef state) { this->position_[idx] = ha_parse_float(state); }

  void store_tilt_(int idx, esphome::StringRef state) { this->tilt_[idx] = ha_parse_float(state); }

  void store_supported_features_(int idx, esphome::StringRef state) {
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
      this->supported_features_[idx] = static_cast<int>(value);
    }
  }

  int find_index_(const std::string &entity_id) const { return find_entity_index(COVER_LIST, COVER_LIST_COUNT, entity_id); }

  std::array<std::string, COVER_LIST_COUNT> state_{};
  std::array<float, COVER_LIST_COUNT> position_ = filled_array<float, COVER_LIST_COUNT>(NAN);
  std::array<float, COVER_LIST_COUNT> tilt_ = filled_array<float, COVER_LIST_COUNT>(NAN);
  std::array<int, COVER_LIST_COUNT> supported_features_{};
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
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_shuffle_, entity_id, "shuffle");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_repeat_, entity_id, "repeat");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_sound_mode_, entity_id, "sound_mode");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_sound_mode_list_, entity_id, "sound_mode_list");
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

  const std::string &shuffle(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) return empty_string();
    return this->shuffle_[idx];
  }

  const std::string &repeat(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) return empty_string();
    return this->repeat_[idx];
  }

  const std::string &sound_mode(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) return empty_string();
    return this->sound_mode_[idx];
  }

  const std::string &sound_mode_list(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) return empty_string();
    return this->sound_mode_list_[idx];
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

  void on_shuffle_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_shuffle_(idx, state);
  }

  void on_repeat_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_repeat_(idx, state);
  }

  void on_sound_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_sound_mode_(idx, state);
  }

  void on_sound_mode_list_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_sound_mode_list_(idx, state);
  }

  void store_state_(int idx, esphome::StringRef state) {
    ha_assign_state_or_unknown(this->state_[idx], state);
  }

  void store_title_(int idx, esphome::StringRef state) { ha_assign(this->title_[idx], state); }

  void store_artist_(int idx, esphome::StringRef state) { ha_assign(this->artist_[idx], state); }

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

  void store_device_class_(int idx, esphome::StringRef state) { ha_assign(this->device_class_[idx], state); }

  void store_source_list_(int idx, esphome::StringRef state) { ha_store_joined_list(this->source_list_[idx], state); }

  void store_volume_(int idx, esphome::StringRef state) {
    this->volume_[idx] = ha_parse_float(state);
  }

  void store_shuffle_(int idx, esphome::StringRef state) { ha_assign(this->shuffle_[idx], state); }

  void store_repeat_(int idx, esphome::StringRef state) { ha_assign(this->repeat_[idx], state); }

  void store_sound_mode_(int idx, esphome::StringRef state) { ha_assign(this->sound_mode_[idx], state); }

  void store_sound_mode_list_(int idx, esphome::StringRef state) {
    ha_store_joined_list(this->sound_mode_list_[idx], state);
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
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> shuffle_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> repeat_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> sound_mode_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> sound_mode_list_{};
  std::array<float, MEDIA_PLAYER_LIST_COUNT> volume_ = filled_array<float, MEDIA_PLAYER_LIST_COUNT>(NAN);
};

class WaterHeaterStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < WATER_HEATER_LIST_COUNT; i++) {
      const char *entity_id = WATER_HEATER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&WaterHeaterStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&WaterHeaterStatusTracker::on_target_temperature_, entity_id, "temperature");
      this->subscribe_homeassistant_state(&WaterHeaterStatusTracker::on_operation_mode_, entity_id, "operation_mode");
      this->subscribe_homeassistant_state(&WaterHeaterStatusTracker::on_operation_list_, entity_id, "operation_list");
      this->subscribe_homeassistant_state(&WaterHeaterStatusTracker::on_away_mode_, entity_id, "away_mode");
    }
  }

  const std::string &state(int idx) const { return (idx >= 0 && idx < WATER_HEATER_LIST_COUNT) ? this->state_[idx] : unknown_string(); }
  float target_temperature(int idx) const { return (idx >= 0 && idx < WATER_HEATER_LIST_COUNT) ? this->target_temperature_[idx] : NAN; }
  const std::string &operation_mode(int idx) const { return (idx >= 0 && idx < WATER_HEATER_LIST_COUNT) ? this->operation_mode_[idx] : empty_string(); }
  const std::string &operation_list(int idx) const { return (idx >= 0 && idx < WATER_HEATER_LIST_COUNT) ? this->operation_list_[idx] : empty_string(); }
  const std::string &away_mode(int idx) const { return (idx >= 0 && idx < WATER_HEATER_LIST_COUNT) ? this->away_mode_[idx] : empty_string(); }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) { int idx = this->find_index_(entity_id); if (idx >= 0) this->store_state_(idx, state); }
  void on_target_temperature_(const std::string &entity_id, esphome::StringRef state) { int idx = this->find_index_(entity_id); if (idx >= 0) this->store_target_temperature_(idx, state); }
  void on_operation_mode_(const std::string &entity_id, esphome::StringRef state) { int idx = this->find_index_(entity_id); if (idx >= 0) this->store_operation_mode_(idx, state); }
  void on_operation_list_(const std::string &entity_id, esphome::StringRef state) { int idx = this->find_index_(entity_id); if (idx >= 0) this->store_operation_list_(idx, state); }
  void on_away_mode_(const std::string &entity_id, esphome::StringRef state) { int idx = this->find_index_(entity_id); if (idx >= 0) this->store_away_mode_(idx, state); }
  void store_state_(int idx, esphome::StringRef state) { ha_assign_state_or_unknown(this->state_[idx], state); }
  void store_target_temperature_(int idx, esphome::StringRef state) { this->target_temperature_[idx] = ha_parse_float(state); }
  void store_operation_mode_(int idx, esphome::StringRef state) { ha_assign(this->operation_mode_[idx], state); }
  void store_operation_list_(int idx, esphome::StringRef state) { ha_store_joined_list(this->operation_list_[idx], state); }
  void store_away_mode_(int idx, esphome::StringRef state) { ha_assign(this->away_mode_[idx], state); }
  int find_index_(const std::string &entity_id) const { return find_entity_index(WATER_HEATER_LIST, WATER_HEATER_LIST_COUNT, entity_id); }
  std::array<std::string, WATER_HEATER_LIST_COUNT> state_{};
  std::array<float, WATER_HEATER_LIST_COUNT> target_temperature_ = filled_array<float, WATER_HEATER_LIST_COUNT>(NAN);
  std::array<std::string, WATER_HEATER_LIST_COUNT> operation_mode_{};
  std::array<std::string, WATER_HEATER_LIST_COUNT> operation_list_{};
  std::array<std::string, WATER_HEATER_LIST_COUNT> away_mode_{};
};

using AutomationStatusTracker = SingleStateTracker<EntityEntry, AUTOMATION_LIST_COUNT>;
using AlarmStatusTracker = SingleStateTracker<EntityEntry, ALARM_LIST_COUNT>;

class WeatherStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < WEATHER_LIST_COUNT; i++) {
      const char *entity_id = WEATHER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_temperature_, entity_id, "temperature");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_humidity_, entity_id, "humidity");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_forecast_, entity_id, "forecast");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_wind_speed_, entity_id, "wind_speed");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_wind_bearing_, entity_id, "wind_bearing");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_wind_gust_speed_, entity_id, "wind_gust_speed");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_pressure_, entity_id, "pressure");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_cloud_coverage_, entity_id, "cloud_coverage");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_uv_index_, entity_id, "uv_index");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_dew_point_, entity_id, "dew_point");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_apparent_temperature_, entity_id, "apparent_temperature");
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

  float wind_speed(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->wind_speed_[idx];
  }

  float wind_bearing(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->wind_bearing_[idx];
  }

  float wind_gust_speed(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->wind_gust_speed_[idx];
  }

  float pressure(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->pressure_[idx];
  }

  float cloud_coverage(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->cloud_coverage_[idx];
  }

  float uv_index(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->uv_index_[idx];
  }

  float dew_point(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->dew_point_[idx];
  }

  float apparent_temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->apparent_temperature_[idx];
  }

  float precipitation(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) return NAN;
    return this->precipitation_[idx];
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

  void on_wind_speed_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_wind_speed_(idx, state);
  }

  void on_wind_bearing_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_wind_bearing_(idx, state);
  }

  void on_wind_gust_speed_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_wind_gust_speed_(idx, state);
  }

  void on_pressure_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_pressure_(idx, state);
  }

  void on_cloud_coverage_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_cloud_coverage_(idx, state);
  }

  void on_uv_index_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_uv_index_(idx, state);
  }

  void on_dew_point_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_dew_point_(idx, state);
  }

  void on_apparent_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) this->store_apparent_temperature_(idx, state);
  }

  void store_state_(int idx, esphome::StringRef state) {
    ha_assign_state_or_unknown(this->state_[idx], state);
  }

  void store_temperature_(int idx, esphome::StringRef state) {
    this->temperature_[idx] = ha_parse_float(state);
  }

  void store_humidity_(int idx, esphome::StringRef state) {
    this->humidity_[idx] = ha_parse_float(state);
  }

  void store_wind_speed_(int idx, esphome::StringRef state) {
    this->wind_speed_[idx] = ha_parse_float(state);
  }

  void store_wind_bearing_(int idx, esphome::StringRef state) {
    this->wind_bearing_[idx] = ha_parse_float(state);
  }

  void store_wind_gust_speed_(int idx, esphome::StringRef state) {
    this->wind_gust_speed_[idx] = ha_parse_float(state);
  }

  void store_pressure_(int idx, esphome::StringRef state) {
    this->pressure_[idx] = ha_parse_float(state);
  }

  void store_cloud_coverage_(int idx, esphome::StringRef state) {
    this->cloud_coverage_[idx] = ha_parse_float(state);
  }

  void store_uv_index_(int idx, esphome::StringRef state) {
    this->uv_index_[idx] = ha_parse_float(state);
  }

  void store_dew_point_(int idx, esphome::StringRef state) {
    this->dew_point_[idx] = ha_parse_float(state);
  }

  void store_apparent_temperature_(int idx, esphome::StringRef state) {
    this->apparent_temperature_[idx] = ha_parse_float(state);
  }

  void store_forecast_(int idx, esphome::StringRef state) {
    this->high_temperature_[idx] = NAN;
    this->low_temperature_[idx] = NAN;
    this->precipitation_[idx] = NAN;

    if (ha_state_missing(state) || !ha_payload_looks_like_json(state)) {
      return;
    }

    // The forecast attribute is routinely 5-30 KB; a filter keeps only the few
    // keys read below so the parsed document stays small, and parsing directly
    // from the StringRef avoids copying the payload. The payload is either a
    // bare array or {"forecast": [...]} — the filter must match that shape.
    size_t first = 0;
    while (first < state.size() && (state[first] == ' ' || state[first] == '\t' ||
                                    state[first] == '\r' || state[first] == '\n')) {
      first++;
    }
    JsonDocument filter;
    JsonObject entry =
        state[first] == '[' ? filter[0].to<JsonObject>() : filter["forecast"][0].to<JsonObject>();
    for (const char *key : {"temperature", "temperature_high", "native_temperature", "native_temperature_high",
                            "templow", "temperature_low", "native_templow", "native_temperature_low",
                            "precipitation", "native_precipitation"}) {
      entry[key] = true;
    }

    JsonDocument doc;
    if (deserializeJson(doc, state.c_str(), state.size(), DeserializationOption::Filter(filter)) !=
        DeserializationError::Ok) {
      return;
    }
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

    float precip = read_number(today, "precipitation");
    if (std::isnan(precip)) precip = read_number(today, "native_precipitation");

    this->high_temperature_[idx] = high;
    this->low_temperature_[idx] = low;
    this->precipitation_[idx] = precip;
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
    idx = clamp_mode_index(idx, this->count_);
    snprintf(buffer, buffer_size, "%d OF %d", idx + 1, this->count_);
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

  void on_payload_(esphome::StringRef state) {
    std::string payload = trim_copy(state.str());
    this->count_ = parse_delimited_slots_(payload, this->messages_);
  }

  void on_ids_payload_(esphome::StringRef state) {
    std::string payload = trim_copy(state.str());
    parse_delimited_slots_(payload, this->ids_);
  }

  int count_{0};
  std::string messages_[NOTIFICATION_FEED_MAX_ITEMS];
  std::string ids_[NOTIFICATION_FEED_MAX_ITEMS];
};
