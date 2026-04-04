#include "remote_ui_sync.h"

namespace esphome {

static inline bool assign_string_if_changed(std::string *target, const std::string &value) {
  if (*target != value) {
    *target = value;
    return true;
  }
  return false;
}

static inline bool assign_int_if_changed(int *target, int value) {
  if (*target != value) {
    *target = value;
    return true;
  }
  return false;
}

static inline bool assign_float_if_changed(float *target, float value) {
  if (!std::isnan(value) && *target != value) {
    *target = value;
    return true;
  }
  return false;
}

static inline void request_refresh(RemoteUiSyncState &ui, int idx) {
  ui.refresh_requested = true;
  ui.refresh_idx = idx;
}

static inline void sync_simple_state(RemoteUiSyncState &ui, const std::string &state, int idx) {
  if (state == "unknown") {
    request_refresh(ui, idx);
    return;
  }
  *ui.updated_ui = assign_string_if_changed(ui.selected_item_state, state) || *ui.updated_ui;
}

void sync_remote_ui_state(RemoteMode mode, int idx, RemoteUiSyncState &ui) {
  if (mode == REMOTE_MODE_LIGHTS) {
    const std::string &state = selected_light_state(idx);
    float brightness = selected_light_brightness(idx);
    std::string next_state = *ui.selected_item_state;
    int next_brightness_pct = *ui.selected_brightness_pct;

    if (state == "on") {
      next_state = "on";
      if (selected_light_has_brightness(idx) && !std::isnan(brightness) && brightness > 0) {
        next_brightness_pct = clamp_percent_value(brightness, 100.0f / 255.0f, 1);
      } else if (next_brightness_pct <= 0) {
        next_brightness_pct = 100;
      }
    } else if (state == "off") {
      next_state = "off";
      next_brightness_pct = 0;
    } else {
      next_state = "unknown";
      request_refresh(ui, idx);
    }

    if (next_state != *ui.selected_item_state || next_brightness_pct != *ui.selected_brightness_pct) {
      *ui.selected_item_state = next_state;
      *ui.selected_brightness_pct = next_brightness_pct;
      *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_FANS) {
    const std::string &state = selected_fan_state(idx);
    float percentage = selected_fan_percentage(idx);
    bool has_percentage = selected_fan_has_percentage(idx);
    std::string next_state = *ui.selected_item_state;
    int next_speed_pct = *ui.selected_fan_speed_pct;

    if (state == "on") {
      next_state = "on";
      if (has_percentage && !std::isnan(percentage) && percentage > 0) {
        next_speed_pct = clamp_percent_value(percentage, 1.0f, 1);
      } else if (!has_percentage) {
        next_speed_pct = 0;
      }
    } else if (state == "off") {
      next_state = "off";
      next_speed_pct = 0;
    } else {
      next_state = "unknown";
      request_refresh(ui, idx);
    }

    if (next_state != *ui.selected_item_state || next_speed_pct != *ui.selected_fan_speed_pct) {
      *ui.selected_item_state = next_state;
      *ui.selected_fan_speed_pct = next_speed_pct;
      *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_HUMIDIFIERS) {
    const std::string &state = selected_humidifier_state(idx);
    const std::string &mode_value = humidifier_mode_for_index(idx);
    const std::string &action = humidifier_action_for_index(idx);
    float target = humidifier_target_humidity_for_index(idx);
    float current = humidifier_current_humidity_for_index(idx);
    bool changed = false;

    if (state == "unknown" && std::isnan(target) && std::isnan(current)) {
      request_refresh(ui, idx);
    } else {
      changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      changed = assign_string_if_changed(ui.selected_humidifier_action, action) || changed;
      changed = assign_string_if_changed(ui.selected_humidifier_mode, mode_value) || changed;
      changed = assign_float_if_changed(ui.selected_humidifier_target_humidity, target) || changed;
      changed = assign_float_if_changed(ui.selected_humidifier_current_humidity, current) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_SWITCHES) {
    sync_simple_state(ui, selected_switch_state(idx), idx);
    return;
  }

  if (mode == REMOTE_MODE_CLIMATE) {
    const std::string &state = selected_climate_state(idx);
    const std::string &hvac_action = climate_hvac_action_for_index(idx);
    const std::string &preset = selected_climate_preset_mode(idx);
    float target = selected_climate_target_temperature(idx);
    float target_low = selected_climate_target_temperature_low(idx);
    float target_high = selected_climate_target_temperature_high(idx);
    float current = selected_climate_current_temperature(idx);
    bool changed = false;

    if (state == "unknown") {
      request_refresh(ui, idx);
    } else {
      changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      if (state != "off") {
        changed = assign_string_if_changed(ui.last_climate_mode, state) || changed;
      }
      if (!hvac_action.empty()) {
        changed = assign_string_if_changed(ui.selected_climate_hvac_action, hvac_action) || changed;
      }
      changed = assign_float_if_changed(ui.selected_climate_target_temp, target) || changed;
      changed = assign_float_if_changed(ui.selected_climate_target_temp_low, target_low) || changed;
      changed = assign_float_if_changed(ui.selected_climate_target_temp_high, target_high) || changed;
      changed = assign_float_if_changed(ui.selected_climate_current_temp, current) || changed;
      changed = assign_string_if_changed(ui.selected_climate_preset, preset) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_LOCKS) {
    sync_simple_state(ui, selected_lock_state(idx), idx);
    return;
  }

  if (mode == REMOTE_MODE_COVERS) {
    const std::string &state = selected_cover_state(idx);
    float position = selected_cover_position(idx);
    bool changed = false;

    if (state == "unknown" && std::isnan(position)) {
      request_refresh(ui, idx);
    } else {
      if (state != "unknown") {
        changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      }
      if (!std::isnan(position)) {
        changed = assign_int_if_changed(ui.selected_cover_position_pct, clamp_percent_value(position)) || changed;
      }
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_MEDIA) {
    const std::string &state = selected_media_state(idx);
    const std::string &device_class = media_device_class_for_index(idx);
    const std::string &title = media_title_for_index(idx);
    const std::string &artist = media_artist_for_index(idx);
    const std::string &source = media_source_for_index(idx);
    float volume = selected_media_volume(idx);
    bool changed = false;

    if (state == "unknown") {
      request_refresh(ui, idx);
    } else {
      changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      changed = assign_string_if_changed(ui.selected_media_title, title) || changed;
      changed = assign_string_if_changed(ui.selected_media_artist, artist) || changed;
      changed = assign_string_if_changed(ui.selected_media_device_class, device_class) || changed;
      if (device_class == "tv" || device_class == "receiver") {
        if (!source.empty()) {
          changed = assign_string_if_changed(ui.selected_media_source, source) || changed;
        }
      } else {
        changed = assign_string_if_changed(ui.selected_media_source, source) || changed;
      }
      if (!std::isnan(volume)) {
        changed = assign_int_if_changed(ui.selected_media_volume_pct, clamp_percent_value(volume, 100.0f)) || changed;
      }
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_SENSORS) {
    sync_simple_state(ui, sensor_state_for_index(idx), idx);
    return;
  }

  if (mode == REMOTE_MODE_AUTOMATION) {
    const std::string &state = automation_state_for_index(idx);
    std::string next_state = automation_supports_enabled_state(idx) ? state : "ready";
    bool changed = false;
    if (automation_supports_enabled_state(idx)) {
      changed = assign_string_if_changed(ui.selected_automation_state, next_state) || changed;
    } else {
      changed = assign_string_if_changed(ui.selected_automation_state, "ready") || changed;
    }
    changed = assign_string_if_changed(ui.selected_item_state, next_state) || changed;
    if (state == "unknown") {
      request_refresh(ui, idx);
    }
    if (changed) *ui.updated_ui = true;
    return;
  }

  if (mode == REMOTE_MODE_ALARMS) {
    sync_simple_state(ui, alarm_state_for_index(idx), idx);
    return;
  }

  if (mode == REMOTE_MODE_NOTIFICATIONS) {
    int count = notification_mode_item_count();
    int clamped_idx = clamp_mode_index(idx, count);
    std::string next_name = notification_mode_item_name(clamped_idx);
    std::string next_entity = notification_mode_item_entity(clamped_idx);
    std::string next_message = notification_message_for_index(clamped_idx);

    if (assign_int_if_changed(ui.selected_notification_index, clamped_idx)) {
      *ui.updated_ui = true;
    }
    *ui.updated_ui = assign_string_if_changed(ui.selected_item_name, next_name) || *ui.updated_ui;
    *ui.updated_ui = assign_string_if_changed(ui.selected_item_entity, next_entity) || *ui.updated_ui;
    *ui.updated_ui = assign_string_if_changed(ui.selected_item_state, next_message) || *ui.updated_ui;
    if (next_entity.empty() || (count > 1 && next_message.empty())) {
      request_refresh(ui, clamped_idx);
    }
    return;
  }

  if (mode == REMOTE_MODE_WEATHER) {
    const std::string &condition = weather_state_for_index(idx);
    float temperature = weather_temperature_for_index(idx);
    float humidity = weather_humidity_for_index(idx);
    float high = weather_high_temperature_for_index(idx);
    float low = weather_low_temperature_for_index(idx);
    bool changed = false;

    if (condition == "unknown" && std::isnan(temperature) && std::isnan(humidity) &&
        std::isnan(high) && std::isnan(low)) {
      request_refresh(ui, idx);
    } else {
      changed = assign_string_if_changed(ui.selected_item_state, condition) || changed;
      changed = assign_string_if_changed(ui.selected_weather_condition, condition) || changed;
      changed = assign_float_if_changed(ui.selected_weather_temperature, temperature) || changed;
      changed = assign_float_if_changed(ui.selected_weather_humidity, humidity) || changed;
      changed = assign_float_if_changed(ui.selected_weather_high_temp, high) || changed;
      changed = assign_float_if_changed(ui.selected_weather_low_temp, low) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_INFO) {
    const char *state = idx == 0 ? "date" : idx == 1 ? "network" : "version";
    *ui.updated_ui = assign_string_if_changed(ui.selected_item_state, state) || *ui.updated_ui;
  }
}

}  // namespace esphome
