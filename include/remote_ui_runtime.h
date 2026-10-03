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

// Held Previous, Next, Minus and Plus repeat their step: the first repeat
// after REPEAT_DELAY_MS, then one every REPEAT_INTERVAL_MS (values) or
// REPEAT_NAVIGATION_INTERVAL_MS (moving through a list).
inline constexpr uint32_t REPEAT_DELAY_MS = 400;
inline constexpr uint32_t REPEAT_INTERVAL_MS = 200;
inline constexpr uint32_t REPEAT_NAVIGATION_INTERVAL_MS = 150;

// True while a held Plus or Minus repeats. send_setting_value then waits, so
// Home Assistant gets the value the hold ends on rather than every step on the
// way there.
inline bool remote_repeat_active = false;

// The buttons that repeat (button_repeat's parameter), and the one repeating.
enum RemoteRepeatButton {
  REMOTE_REPEAT_PREVIOUS = 0,
  REMOTE_REPEAT_NEXT = 1,
  REMOTE_REPEAT_MINUS = 2,
  REMOTE_REPEAT_PLUS = 3,
};
inline int remote_repeat_button = -1;
// The setting and entity a held Plus or Minus started on. send_setting_value
// holds back one value at a time, so the repeat ends when either changes (a
// tap of Settings), letting the value held so far go out.
inline int remote_repeat_setting = 0;
inline std::string remote_repeat_entity;

// Set by shift_selected_item when it selected another item.
inline bool shift_selected_item_moved = false;

// An HVAC mode or a select option goes out this long after the last press
// that stepped it (see send_option_after_pause).
inline constexpr uint32_t OPTION_SEND_DELAY_MS = 1500;

// The option waiting to go out: kind is a RemoteOptionSend, -1 for none.
struct RemotePendingOption {
  int kind = -1;
  std::string entity;
  std::string option;
};
inline RemotePendingOption remote_pending_option;

// The screen dims this long before the remote goes to sleep, as a warning; any
// button brings it back. Only when SLEEP_DURATION leaves room for it.
inline constexpr uint32_t IDLE_DIM_BEFORE_SLEEP_S = 10;

inline bool idle_dim_due(uint32_t idle_s, uint32_t sleep_after_s) {
  return sleep_after_s > 2 * IDLE_DIM_BEFORE_SLEEP_S && idle_s + IDLE_DIM_BEFORE_SLEEP_S >= sleep_after_s;
}

// The value the remote just sent for one setting of one entity (a light's
// colour temperature, a number), which the screen shows and the next press
// steps from until Home Assistant reports it, as cover tilt does above.
struct RemotePendingValue {
  int mode = -1;
  int index = -1;
  int setting = 0;
  float value = NAN;
  uint32_t at = 0;
};
inline RemotePendingValue remote_pending_value;

inline void note_pending_value(int mode, int index, int setting, float value) {
  remote_pending_value = {mode, index, setting, value, millis() | 1};
}
// The pending value for this setting, or fallback when there is none.
inline float pending_value_or(uint32_t now, int mode, int index, int setting, float fallback) {
  const RemotePendingValue &p = remote_pending_value;
  bool active = p.at != 0 && p.mode == mode && p.index == index && p.setting == setting &&
                now - p.at < LOCAL_CHANGE_HOLD_MS;
  return active ? p.value : fallback;
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
