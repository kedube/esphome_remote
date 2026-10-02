#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "esphome/core/hal.h"

namespace esphome {

// Hold on syncing after a local change. Actions write the expected result into
// the UI globals as soon as they send a command, but the periodic sync would
// copy Home Assistant's not-yet-updated value straight back over it within
// 500 ms: the screen jumped back, and a second press stepped from the stale
// value and re-sent the first one. While the hold is active the sync leaves the
// selected item alone; once it lapses Home Assistant's value wins again, so a
// rejected command still reverts. Selecting another item clears the hold.
inline constexpr uint32_t LOCAL_CHANGE_HOLD_MS = 3000;
inline uint32_t local_change_at = 0;

inline void note_local_change() { local_change_at = millis() | 1; }  // 0 means "no hold"
inline void clear_local_change() { local_change_at = 0; }
inline bool local_change_hold_active(uint32_t now) {
  return local_change_at != 0 && now - local_change_at < LOCAL_CHANGE_HOLD_MS;
}

// The cover whose tilt the remote just sent. Its pending_cover_tilt_pct is what
// the next press steps from, and what the screen shows, until Home Assistant
// catches up: only for that cover, and only within the local-change hold.
inline int cover_tilt_sent_index = -1;
inline uint32_t cover_tilt_sent_at = 0;

inline void note_cover_tilt_sent(int cover_index) {
  cover_tilt_sent_index = cover_index;
  cover_tilt_sent_at = millis() | 1;  // 0 means "nothing sent"
}
inline bool cover_tilt_sent_recently(uint32_t now, int cover_index) {
  return cover_tilt_sent_at != 0 && cover_tilt_sent_index == cover_index &&
         now - cover_tilt_sent_at < LOCAL_CHANGE_HOLD_MS;
}

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
  bool *updated_ui = nullptr;
};

// Pointers to the interaction-timestamp globals. apply_remote_ui_timeout_updates
// zeroes each timestamp when its feedback window expires (a fire-once latch),
// so the fields must reference the globals rather than hold copies.
struct RemoteUiTimeoutState {
  uint32_t *last_brightness_interaction = nullptr;
  uint32_t *last_switch_interaction = nullptr;
  uint32_t *last_contrast_interaction = nullptr;
  uint32_t *last_setting_interaction = nullptr;
  uint32_t *last_climate_interaction = nullptr;
  uint32_t *last_fan_speed_interaction = nullptr;
  uint32_t *last_humidifier_interaction = nullptr;
  uint32_t *last_humidifier_mode_interaction = nullptr;
  uint32_t *last_lock_interaction = nullptr;
  uint32_t *last_cover_interaction = nullptr;
  uint32_t *last_cover_position_interaction = nullptr;
  uint32_t *last_climate_target_focus_interaction = nullptr;
  uint32_t *last_media_volume_interaction = nullptr;
  uint32_t *last_media_source_interaction = nullptr;
  uint32_t *last_media_power_interaction = nullptr;
  uint32_t *last_automation_interaction = nullptr;
  uint32_t *last_alarm_interaction = nullptr;
  uint32_t *ui_toast_at = nullptr;
  int *climate_target_focus = nullptr;
  float *climate_target_focus_value = nullptr;
  std::string *selected_setting_detail = nullptr;
  bool preserve_selected_setting_detail = false;
  bool *updated_ui = nullptr;
};

void reset_remote_ui_state(RemoteUiResetState &state);
void apply_remote_ui_timeout_updates(uint32_t now, RemoteUiTimeoutState &state);

}  // namespace esphome
