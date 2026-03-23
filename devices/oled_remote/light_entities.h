#pragma once

#include <cmath>
#include <string>

#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/string_ref.h"

struct LightEntity {
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

static const int LIGHT_LIST_COUNT = sizeof(LIGHT_LIST) / sizeof(LIGHT_LIST[0]);

class LightStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < LIGHT_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&LightStatusTracker::on_light_state_, LIGHT_LIST[i].entity_id);
      this->subscribe_homeassistant_state(&LightStatusTracker::on_light_brightness_, LIGHT_LIST[i].entity_id, "brightness");
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
    static const std::string unknown = "unknown";
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return unknown;
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
    this->state_[idx] = state.str();
    if (this->state_[idx].empty()) {
      this->state_[idx] = "unknown";
      this->has_state_[idx] = false;
    } else {
      this->has_state_[idx] = true;
    }
  }

  void store_brightness_(int idx, esphome::StringRef state) {
    std::string value = state.str();
    if (value.empty() || value == "unknown" || value == "unavailable" || value == "None") {
      this->brightness_[idx] = NAN;
      this->has_brightness_[idx] = false;
      return;
    }

    this->brightness_[idx] = strtof(value.c_str(), nullptr);
    this->has_brightness_[idx] = !std::isnan(this->brightness_[idx]);
  }

  int find_index_(const std::string &entity_id) const {
    for (int i = 0; i < LIGHT_LIST_COUNT; i++) {
      if (entity_id == LIGHT_LIST[i].entity_id) {
        return i;
      }
    }
    return -1;
  }

  std::string state_[LIGHT_LIST_COUNT];
  float brightness_[LIGHT_LIST_COUNT] = {NAN};
  bool has_state_[LIGHT_LIST_COUNT] = {false};
  bool has_brightness_[LIGHT_LIST_COUNT] = {false};
};

static LightStatusTracker light_status_tracker_storage;
static bool light_status_tracker_initialized = false;

static inline void ensure_light_status_tracker() {
  if (light_status_tracker_initialized) {
    return;
  }
  light_status_tracker_storage.setup();
  light_status_tracker_initialized = true;
}

static inline void request_selected_light_status(int idx) {
  ensure_light_status_tracker();
  light_status_tracker_storage.request_light_state(idx);
}

static inline bool selected_light_has_state(int idx) {
  ensure_light_status_tracker();
  return light_status_tracker_storage.has_state(idx);
}

static inline std::string selected_light_state(int idx) {
  ensure_light_status_tracker();
  return light_status_tracker_storage.state(idx);
}

static inline bool selected_light_has_brightness(int idx) {
  ensure_light_status_tracker();
  return light_status_tracker_storage.has_brightness(idx);
}

static inline float selected_light_brightness(int idx) {
  ensure_light_status_tracker();
  return light_status_tracker_storage.brightness(idx);
}
