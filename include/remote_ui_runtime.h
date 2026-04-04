#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace esphome {

struct RemoteUiResetState {
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
  int *climate_target_focus = nullptr;
  float *climate_target_focus_value = nullptr;
  uint32_t *last_climate_target_focus_interaction = nullptr;
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
  bool *updated_ui = nullptr;
};

struct RemoteUiTimeoutState {
  uint32_t last_brightness_interaction = 0;
  uint32_t last_switch_interaction = 0;
  uint32_t last_contrast_interaction = 0;
  uint32_t last_climate_interaction = 0;
  uint32_t last_fan_speed_interaction = 0;
  uint32_t last_humidifier_interaction = 0;
  uint32_t last_humidifier_mode_interaction = 0;
  uint32_t last_lock_interaction = 0;
  uint32_t last_cover_interaction = 0;
  uint32_t last_cover_position_interaction = 0;
  uint32_t last_climate_target_focus_interaction = 0;
  uint32_t last_media_volume_interaction = 0;
  uint32_t last_media_source_interaction = 0;
  uint32_t last_media_power_interaction = 0;
  uint32_t last_automation_interaction = 0;
  uint32_t last_alarm_interaction = 0;
  int climate_target_focus = 0;
  float climate_target_focus_value = NAN;
  bool updated_ui = false;
};

void reset_remote_ui_state(RemoteUiResetState &state);
void apply_remote_ui_timeout_updates(uint32_t now, RemoteUiTimeoutState &state);

}  // namespace esphome
