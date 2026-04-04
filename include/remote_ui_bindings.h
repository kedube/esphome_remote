#pragma once

#include "remote_ui_runtime.h"
#include "remote_ui_sync.h"

namespace esphome {

struct RemoteUiBindings {
  std::string *selected_item_name = nullptr;
  std::string *selected_item_entity = nullptr;
  std::string *selected_item_state = nullptr;
  int *selected_brightness_pct = nullptr;
  int *selected_fan_speed_pct = nullptr;
  float *selected_humidifier_target_humidity = nullptr;
  float *selected_humidifier_current_humidity = nullptr;
  std::string *selected_humidifier_action = nullptr;
  std::string *selected_humidifier_mode = nullptr;
  int *selected_cover_position_pct = nullptr;
  std::string *last_switch_feedback = nullptr;
  uint32_t *last_switch_interaction = nullptr;
  float *selected_climate_target_temp = nullptr;
  std::string *selected_climate_hvac_action = nullptr;
  std::string *selected_climate_preset = nullptr;
  float *selected_climate_target_temp_low = nullptr;
  float *selected_climate_target_temp_high = nullptr;
  float *selected_climate_current_temp = nullptr;
  std::string *last_climate_mode = nullptr;
  int *climate_target_focus = nullptr;
  float *climate_target_focus_value = nullptr;
  uint32_t *last_climate_target_focus_interaction = nullptr;
  int *selected_media_volume_pct = nullptr;
  std::string *selected_media_title = nullptr;
  std::string *selected_media_artist = nullptr;
  std::string *selected_media_device_class = nullptr;
  std::string *selected_media_source = nullptr;
  std::string *last_media_power_feedback = nullptr;
  std::string *selected_automation_state = nullptr;
  std::string *last_alarm_feedback = nullptr;
  uint32_t *last_alarm_interaction = nullptr;
  std::string *last_lock_feedback = nullptr;
  uint32_t *last_lock_interaction = nullptr;
  std::string *last_cover_feedback = nullptr;
  uint32_t *last_cover_interaction = nullptr;
  uint32_t *last_cover_position_interaction = nullptr;
  uint32_t *primary_button_press_started_at = nullptr;
  uint32_t *play_pause_button_press_started_at = nullptr;
  uint32_t *settings_button_press_started_at = nullptr;
  int *primary_button_press_mode = nullptr;
  int *play_pause_button_press_mode = nullptr;
  int *settings_button_press_mode = nullptr;
  bool *primary_button_long_press_fired = nullptr;
  bool *play_pause_button_long_press_fired = nullptr;
  bool *settings_button_long_press_fired = nullptr;
  float *selected_weather_temperature = nullptr;
  float *selected_weather_humidity = nullptr;
  float *selected_weather_high_temp = nullptr;
  float *selected_weather_low_temp = nullptr;
  std::string *selected_weather_condition = nullptr;
  int *selected_notification_index = nullptr;
  bool *updated_ui = nullptr;
};

static inline RemoteUiBindings make_remote_ui_bindings(
    std::string &selected_item_name, std::string &selected_item_entity, std::string &selected_item_state,
    int &selected_brightness_pct, int &selected_fan_speed_pct, float &selected_humidifier_target_humidity,
    float &selected_humidifier_current_humidity, std::string &selected_humidifier_action,
    std::string &selected_humidifier_mode, int &selected_cover_position_pct, std::string &last_switch_feedback,
    uint32_t &last_switch_interaction, float &selected_climate_target_temp,
    std::string &selected_climate_hvac_action, std::string &selected_climate_preset,
    float &selected_climate_target_temp_low, float &selected_climate_target_temp_high,
    float &selected_climate_current_temp, std::string &last_climate_mode, int &climate_target_focus,
    float &climate_target_focus_value, uint32_t &last_climate_target_focus_interaction,
    int &selected_media_volume_pct, std::string &selected_media_title, std::string &selected_media_artist,
    std::string &selected_media_device_class, std::string &selected_media_source,
    std::string &last_media_power_feedback, std::string &selected_automation_state,
    std::string &last_alarm_feedback, uint32_t &last_alarm_interaction, std::string &last_lock_feedback,
    uint32_t &last_lock_interaction, std::string &last_cover_feedback, uint32_t &last_cover_interaction,
    uint32_t &last_cover_position_interaction, uint32_t &primary_button_press_started_at,
    uint32_t &play_pause_button_press_started_at, uint32_t &settings_button_press_started_at,
    int &primary_button_press_mode, int &play_pause_button_press_mode, int &settings_button_press_mode,
    bool &primary_button_long_press_fired, bool &play_pause_button_long_press_fired,
    bool &settings_button_long_press_fired, float &selected_weather_temperature,
    float &selected_weather_humidity, float &selected_weather_high_temp, float &selected_weather_low_temp,
    std::string &selected_weather_condition, int &selected_notification_index, bool &updated_ui) {
  RemoteUiBindings bindings;
  bindings.selected_item_name = &selected_item_name;
  bindings.selected_item_entity = &selected_item_entity;
  bindings.selected_item_state = &selected_item_state;
  bindings.selected_brightness_pct = &selected_brightness_pct;
  bindings.selected_fan_speed_pct = &selected_fan_speed_pct;
  bindings.selected_humidifier_target_humidity = &selected_humidifier_target_humidity;
  bindings.selected_humidifier_current_humidity = &selected_humidifier_current_humidity;
  bindings.selected_humidifier_action = &selected_humidifier_action;
  bindings.selected_humidifier_mode = &selected_humidifier_mode;
  bindings.selected_cover_position_pct = &selected_cover_position_pct;
  bindings.last_switch_feedback = &last_switch_feedback;
  bindings.last_switch_interaction = &last_switch_interaction;
  bindings.selected_climate_target_temp = &selected_climate_target_temp;
  bindings.selected_climate_hvac_action = &selected_climate_hvac_action;
  bindings.selected_climate_preset = &selected_climate_preset;
  bindings.selected_climate_target_temp_low = &selected_climate_target_temp_low;
  bindings.selected_climate_target_temp_high = &selected_climate_target_temp_high;
  bindings.selected_climate_current_temp = &selected_climate_current_temp;
  bindings.last_climate_mode = &last_climate_mode;
  bindings.climate_target_focus = &climate_target_focus;
  bindings.climate_target_focus_value = &climate_target_focus_value;
  bindings.last_climate_target_focus_interaction = &last_climate_target_focus_interaction;
  bindings.selected_media_volume_pct = &selected_media_volume_pct;
  bindings.selected_media_title = &selected_media_title;
  bindings.selected_media_artist = &selected_media_artist;
  bindings.selected_media_device_class = &selected_media_device_class;
  bindings.selected_media_source = &selected_media_source;
  bindings.last_media_power_feedback = &last_media_power_feedback;
  bindings.selected_automation_state = &selected_automation_state;
  bindings.last_alarm_feedback = &last_alarm_feedback;
  bindings.last_alarm_interaction = &last_alarm_interaction;
  bindings.last_lock_feedback = &last_lock_feedback;
  bindings.last_lock_interaction = &last_lock_interaction;
  bindings.last_cover_feedback = &last_cover_feedback;
  bindings.last_cover_interaction = &last_cover_interaction;
  bindings.last_cover_position_interaction = &last_cover_position_interaction;
  bindings.primary_button_press_started_at = &primary_button_press_started_at;
  bindings.play_pause_button_press_started_at = &play_pause_button_press_started_at;
  bindings.settings_button_press_started_at = &settings_button_press_started_at;
  bindings.primary_button_press_mode = &primary_button_press_mode;
  bindings.play_pause_button_press_mode = &play_pause_button_press_mode;
  bindings.settings_button_press_mode = &settings_button_press_mode;
  bindings.primary_button_long_press_fired = &primary_button_long_press_fired;
  bindings.play_pause_button_long_press_fired = &play_pause_button_long_press_fired;
  bindings.settings_button_long_press_fired = &settings_button_long_press_fired;
  bindings.selected_weather_temperature = &selected_weather_temperature;
  bindings.selected_weather_humidity = &selected_weather_humidity;
  bindings.selected_weather_high_temp = &selected_weather_high_temp;
  bindings.selected_weather_low_temp = &selected_weather_low_temp;
  bindings.selected_weather_condition = &selected_weather_condition;
  bindings.selected_notification_index = &selected_notification_index;
  bindings.updated_ui = &updated_ui;
  return bindings;
}

static inline RemoteUiResetState make_remote_ui_reset_state(const RemoteUiBindings &bindings) {
  RemoteUiResetState state;
  state.selected_item_state = bindings.selected_item_state;
  state.selected_brightness_pct = bindings.selected_brightness_pct;
  state.selected_fan_speed_pct = bindings.selected_fan_speed_pct;
  state.selected_humidifier_target_humidity = bindings.selected_humidifier_target_humidity;
  state.selected_humidifier_current_humidity = bindings.selected_humidifier_current_humidity;
  state.selected_humidifier_action = bindings.selected_humidifier_action;
  state.selected_humidifier_mode = bindings.selected_humidifier_mode;
  state.selected_cover_position_pct = bindings.selected_cover_position_pct;
  state.last_switch_feedback = bindings.last_switch_feedback;
  state.last_switch_interaction = bindings.last_switch_interaction;
  state.selected_climate_target_temp = bindings.selected_climate_target_temp;
  state.selected_climate_hvac_action = bindings.selected_climate_hvac_action;
  state.selected_climate_preset = bindings.selected_climate_preset;
  state.selected_climate_target_temp_low = bindings.selected_climate_target_temp_low;
  state.selected_climate_target_temp_high = bindings.selected_climate_target_temp_high;
  state.selected_climate_current_temp = bindings.selected_climate_current_temp;
  state.climate_target_focus = bindings.climate_target_focus;
  state.climate_target_focus_value = bindings.climate_target_focus_value;
  state.last_climate_target_focus_interaction = bindings.last_climate_target_focus_interaction;
  state.selected_media_title = bindings.selected_media_title;
  state.selected_media_artist = bindings.selected_media_artist;
  state.selected_media_device_class = bindings.selected_media_device_class;
  state.selected_media_source = bindings.selected_media_source;
  state.last_media_power_feedback = bindings.last_media_power_feedback;
  state.selected_automation_state = bindings.selected_automation_state;
  state.last_alarm_feedback = bindings.last_alarm_feedback;
  state.last_alarm_interaction = bindings.last_alarm_interaction;
  state.last_lock_feedback = bindings.last_lock_feedback;
  state.last_lock_interaction = bindings.last_lock_interaction;
  state.last_cover_feedback = bindings.last_cover_feedback;
  state.last_cover_interaction = bindings.last_cover_interaction;
  state.last_cover_position_interaction = bindings.last_cover_position_interaction;
  state.primary_button_press_started_at = bindings.primary_button_press_started_at;
  state.play_pause_button_press_started_at = bindings.play_pause_button_press_started_at;
  state.settings_button_press_started_at = bindings.settings_button_press_started_at;
  state.primary_button_press_mode = bindings.primary_button_press_mode;
  state.play_pause_button_press_mode = bindings.play_pause_button_press_mode;
  state.settings_button_press_mode = bindings.settings_button_press_mode;
  state.primary_button_long_press_fired = bindings.primary_button_long_press_fired;
  state.play_pause_button_long_press_fired = bindings.play_pause_button_long_press_fired;
  state.settings_button_long_press_fired = bindings.settings_button_long_press_fired;
  state.selected_weather_temperature = bindings.selected_weather_temperature;
  state.selected_weather_humidity = bindings.selected_weather_humidity;
  state.selected_weather_high_temp = bindings.selected_weather_high_temp;
  state.selected_weather_low_temp = bindings.selected_weather_low_temp;
  state.selected_weather_condition = bindings.selected_weather_condition;
  state.updated_ui = bindings.updated_ui;
  return state;
}

static inline RemoteUiSyncState make_remote_ui_sync_state(const RemoteUiBindings &bindings) {
  RemoteUiSyncState state;
  state.selected_item_name = bindings.selected_item_name;
  state.selected_item_entity = bindings.selected_item_entity;
  state.selected_item_state = bindings.selected_item_state;
  state.selected_brightness_pct = bindings.selected_brightness_pct;
  state.selected_fan_speed_pct = bindings.selected_fan_speed_pct;
  state.selected_humidifier_target_humidity = bindings.selected_humidifier_target_humidity;
  state.selected_humidifier_current_humidity = bindings.selected_humidifier_current_humidity;
  state.selected_humidifier_action = bindings.selected_humidifier_action;
  state.selected_humidifier_mode = bindings.selected_humidifier_mode;
  state.selected_cover_position_pct = bindings.selected_cover_position_pct;
  state.selected_climate_target_temp = bindings.selected_climate_target_temp;
  state.selected_climate_target_temp_low = bindings.selected_climate_target_temp_low;
  state.selected_climate_target_temp_high = bindings.selected_climate_target_temp_high;
  state.selected_climate_current_temp = bindings.selected_climate_current_temp;
  state.selected_climate_hvac_action = bindings.selected_climate_hvac_action;
  state.selected_climate_preset = bindings.selected_climate_preset;
  state.last_climate_mode = bindings.last_climate_mode;
  state.selected_media_volume_pct = bindings.selected_media_volume_pct;
  state.selected_media_title = bindings.selected_media_title;
  state.selected_media_artist = bindings.selected_media_artist;
  state.selected_media_device_class = bindings.selected_media_device_class;
  state.selected_media_source = bindings.selected_media_source;
  state.selected_automation_state = bindings.selected_automation_state;
  state.selected_weather_temperature = bindings.selected_weather_temperature;
  state.selected_weather_humidity = bindings.selected_weather_humidity;
  state.selected_weather_high_temp = bindings.selected_weather_high_temp;
  state.selected_weather_low_temp = bindings.selected_weather_low_temp;
  state.selected_weather_condition = bindings.selected_weather_condition;
  state.selected_notification_index = bindings.selected_notification_index;
  state.updated_ui = bindings.updated_ui;
  return state;
}

}  // namespace esphome
