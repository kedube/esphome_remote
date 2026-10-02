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
  float *selected_water_heater_target_temp = nullptr;
  std::string *selected_climate_hvac_action = nullptr;
  std::string *selected_climate_fan_mode = nullptr;
  std::string *selected_climate_preset = nullptr;
  float *selected_climate_target_temp_low = nullptr;
  float *selected_climate_target_temp_high = nullptr;
  float *selected_climate_current_temp = nullptr;
  float *selected_climate_target_humidity = nullptr;
  std::string *selected_water_heater_mode = nullptr;
  std::string *selected_water_heater_away = nullptr;
  int *climate_target_focus = nullptr;
  float *climate_target_focus_value = nullptr;
  uint32_t *last_climate_target_focus_interaction = nullptr;
  int *selected_media_volume_pct = nullptr;
  std::string *selected_media_title = nullptr;
  std::string *selected_media_artist = nullptr;
  std::string *selected_media_device_class = nullptr;
  std::string *selected_media_source = nullptr;
  std::string *selected_media_shuffle = nullptr;
  std::string *selected_media_repeat = nullptr;
  std::string *selected_media_sound_mode = nullptr;
  std::string *last_media_power_feedback = nullptr;
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
  float *selected_weather_wind_speed = nullptr;
  float *selected_weather_wind_bearing = nullptr;
  float *selected_weather_wind_gust_speed = nullptr;
  float *selected_weather_pressure = nullptr;
  float *selected_weather_cloud_coverage = nullptr;
  float *selected_weather_uv_index = nullptr;
  float *selected_weather_dew_point = nullptr;
  float *selected_weather_apparent_temperature = nullptr;
  float *selected_weather_precipitation = nullptr;
  std::string *selected_weather_condition = nullptr;
  std::string *selected_sensor_unit = nullptr;
  std::string *selected_setting_detail = nullptr;
  int *selected_notification_index = nullptr;
  bool *updated_ui = nullptr;
};

// Pointers to the ESPHome globals the C++ UI helpers read and write. Globals
// are only reachable through id() inside YAML lambdas, so on_boot fills this in
// once (by field name) and every script reads it from here. The pointers stay
// valid for the life of the program.
inline RemoteUiBindings remote_ui_bindings;
inline bool remote_ui_bindings_bound = false;

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
  state.selected_water_heater_target_temp = bindings.selected_water_heater_target_temp;
  state.selected_climate_hvac_action = bindings.selected_climate_hvac_action;
  state.selected_climate_fan_mode = bindings.selected_climate_fan_mode;
  state.selected_climate_preset = bindings.selected_climate_preset;
  state.selected_climate_target_temp_low = bindings.selected_climate_target_temp_low;
  state.selected_climate_target_temp_high = bindings.selected_climate_target_temp_high;
  state.selected_climate_current_temp = bindings.selected_climate_current_temp;
  state.selected_climate_target_humidity = bindings.selected_climate_target_humidity;
  state.selected_water_heater_mode = bindings.selected_water_heater_mode;
  state.selected_water_heater_away = bindings.selected_water_heater_away;
  state.climate_target_focus = bindings.climate_target_focus;
  state.climate_target_focus_value = bindings.climate_target_focus_value;
  state.last_climate_target_focus_interaction = bindings.last_climate_target_focus_interaction;
  state.selected_media_title = bindings.selected_media_title;
  state.selected_media_artist = bindings.selected_media_artist;
  state.selected_media_device_class = bindings.selected_media_device_class;
  state.selected_media_source = bindings.selected_media_source;
  state.selected_media_shuffle = bindings.selected_media_shuffle;
  state.selected_media_repeat = bindings.selected_media_repeat;
  state.selected_media_sound_mode = bindings.selected_media_sound_mode;
  state.last_media_power_feedback = bindings.last_media_power_feedback;
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
  state.selected_weather_wind_speed = bindings.selected_weather_wind_speed;
  state.selected_weather_wind_bearing = bindings.selected_weather_wind_bearing;
  state.selected_weather_wind_gust_speed = bindings.selected_weather_wind_gust_speed;
  state.selected_weather_pressure = bindings.selected_weather_pressure;
  state.selected_weather_cloud_coverage = bindings.selected_weather_cloud_coverage;
  state.selected_weather_uv_index = bindings.selected_weather_uv_index;
  state.selected_weather_dew_point = bindings.selected_weather_dew_point;
  state.selected_weather_apparent_temperature = bindings.selected_weather_apparent_temperature;
  state.selected_weather_precipitation = bindings.selected_weather_precipitation;
  state.selected_weather_condition = bindings.selected_weather_condition;
  state.selected_sensor_unit = bindings.selected_sensor_unit;
  state.selected_setting_detail = bindings.selected_setting_detail;
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
  state.selected_water_heater_target_temp = bindings.selected_water_heater_target_temp;
  state.selected_climate_target_temp_low = bindings.selected_climate_target_temp_low;
  state.selected_climate_target_temp_high = bindings.selected_climate_target_temp_high;
  state.selected_climate_current_temp = bindings.selected_climate_current_temp;
  state.selected_climate_target_humidity = bindings.selected_climate_target_humidity;
  state.selected_climate_hvac_action = bindings.selected_climate_hvac_action;
  state.selected_climate_fan_mode = bindings.selected_climate_fan_mode;
  state.selected_climate_preset = bindings.selected_climate_preset;
  state.selected_water_heater_mode = bindings.selected_water_heater_mode;
  state.selected_water_heater_away = bindings.selected_water_heater_away;
  state.selected_media_volume_pct = bindings.selected_media_volume_pct;
  state.selected_media_title = bindings.selected_media_title;
  state.selected_media_artist = bindings.selected_media_artist;
  state.selected_media_device_class = bindings.selected_media_device_class;
  state.selected_media_source = bindings.selected_media_source;
  state.selected_media_shuffle = bindings.selected_media_shuffle;
  state.selected_media_repeat = bindings.selected_media_repeat;
  state.selected_media_sound_mode = bindings.selected_media_sound_mode;
  state.selected_sensor_unit = bindings.selected_sensor_unit;
  state.selected_weather_temperature = bindings.selected_weather_temperature;
  state.selected_weather_humidity = bindings.selected_weather_humidity;
  state.selected_weather_high_temp = bindings.selected_weather_high_temp;
  state.selected_weather_low_temp = bindings.selected_weather_low_temp;
  state.selected_weather_wind_speed = bindings.selected_weather_wind_speed;
  state.selected_weather_wind_bearing = bindings.selected_weather_wind_bearing;
  state.selected_weather_wind_gust_speed = bindings.selected_weather_wind_gust_speed;
  state.selected_weather_pressure = bindings.selected_weather_pressure;
  state.selected_weather_cloud_coverage = bindings.selected_weather_cloud_coverage;
  state.selected_weather_uv_index = bindings.selected_weather_uv_index;
  state.selected_weather_dew_point = bindings.selected_weather_dew_point;
  state.selected_weather_apparent_temperature = bindings.selected_weather_apparent_temperature;
  state.selected_weather_precipitation = bindings.selected_weather_precipitation;
  state.selected_weather_condition = bindings.selected_weather_condition;
  state.selected_notification_index = bindings.selected_notification_index;
  state.updated_ui = bindings.updated_ui;
  return state;
}

}  // namespace esphome
