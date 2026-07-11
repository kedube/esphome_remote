#include "remote_ui_runtime.h"

namespace esphome {

void reset_remote_ui_state(RemoteUiResetState &state) {
  *state.selected_item_state = "unknown";
  *state.selected_brightness_pct = 0;
  *state.selected_fan_speed_pct = 0;
  *state.selected_humidifier_target_humidity = NAN;
  *state.selected_humidifier_current_humidity = NAN;
  *state.selected_humidifier_action = "unknown";
  state.selected_humidifier_mode->clear();
  *state.selected_cover_position_pct = 0;
  state.last_switch_feedback->clear();
  *state.last_switch_interaction = 0;
  *state.selected_climate_target_temp = NAN;
  *state.selected_water_heater_target_temp = NAN;
  *state.selected_climate_hvac_action = "unknown";
  state.selected_climate_fan_mode->clear();
  state.selected_climate_hvac_mode->clear();
  *state.selected_climate_target_temp_low = NAN;
  *state.selected_climate_target_temp_high = NAN;
  *state.selected_climate_current_temp = NAN;
  *state.selected_climate_target_humidity = NAN;
  state.selected_water_heater_mode->clear();
  state.selected_water_heater_away->clear();
  *state.climate_target_focus = 0;
  *state.climate_target_focus_value = NAN;
  *state.last_climate_target_focus_interaction = 0;
  state.selected_media_title->clear();
  state.selected_media_artist->clear();
  state.selected_media_device_class->clear();
  state.selected_media_source->clear();
  state.selected_media_shuffle->clear();
  state.selected_media_repeat->clear();
  state.selected_media_sound_mode->clear();
  state.last_media_power_feedback->clear();
  state.last_alarm_feedback->clear();
  *state.last_alarm_interaction = 0;
  state.last_lock_feedback->clear();
  *state.last_lock_interaction = 0;
  state.last_cover_feedback->clear();
  *state.last_cover_interaction = 0;
  *state.last_cover_position_interaction = 0;
  *state.primary_button_press_started_at = 0;
  *state.play_pause_button_press_started_at = 0;
  *state.settings_button_press_started_at = 0;
  *state.primary_button_press_mode = -1;
  *state.play_pause_button_press_mode = -1;
  *state.settings_button_press_mode = -1;
  *state.primary_button_long_press_fired = false;
  *state.play_pause_button_long_press_fired = false;
  *state.settings_button_long_press_fired = false;
  *state.selected_weather_temperature = NAN;
  *state.selected_weather_humidity = NAN;
  *state.selected_weather_high_temp = NAN;
  *state.selected_weather_low_temp = NAN;
  *state.selected_weather_wind_speed = NAN;
  *state.selected_weather_wind_bearing = NAN;
  *state.selected_weather_wind_gust_speed = NAN;
  *state.selected_weather_pressure = NAN;
  *state.selected_weather_cloud_coverage = NAN;
  *state.selected_weather_uv_index = NAN;
  *state.selected_weather_dew_point = NAN;
  *state.selected_weather_apparent_temperature = NAN;
  *state.selected_weather_precipitation = NAN;
  *state.selected_weather_condition = "unknown";
  state.selected_sensor_unit->clear();
  state.selected_setting_detail->clear();
  *state.updated_ui = true;
}

// Fire-once latch: returns true exactly once after the timeout elapses and
// zeroes the timestamp. Unlike a fixed sampling window, a delayed poll tick
// cannot skip past it and leave stale feedback on screen.
static inline bool timeout_expired_once(uint32_t now, uint32_t *last_interaction, uint32_t timeout_ms) {
  if (last_interaction == nullptr || *last_interaction == 0) {
    return false;
  }
  if (now - *last_interaction < timeout_ms) {
    return false;
  }
  *last_interaction = 0;
  return true;
}

void apply_remote_ui_timeout_updates(uint32_t now, RemoteUiTimeoutState &state) {
  struct TimeoutRefreshWindow {
    uint32_t *last_interaction;
    uint32_t timeout_ms;
  };
  const TimeoutRefreshWindow refresh_windows[] = {
      {state.last_brightness_interaction, 3000},
      {state.last_switch_interaction, 5000},
      {state.last_fan_speed_interaction, 3000},
      {state.last_humidifier_interaction, 5000},
      {state.last_humidifier_mode_interaction, 5000},
      {state.last_climate_interaction, 3000},
      {state.last_lock_interaction, 5000},
      {state.last_cover_interaction, 5000},
      {state.last_cover_position_interaction, 3000},
      {state.last_media_volume_interaction, 3000},
      {state.last_media_source_interaction, 5000},
      {state.last_media_power_interaction, 5000},
      {state.last_automation_interaction, 5000},
      {state.last_alarm_interaction, 5000},
      {state.last_contrast_interaction, 5000},
  };
  for (const auto &window : refresh_windows) {
    if (timeout_expired_once(now, window.last_interaction, window.timeout_ms)) {
      *state.updated_ui = true;
    }
  }
  if (timeout_expired_once(now, state.last_setting_interaction, 5000)) {
    if (!state.preserve_selected_setting_detail && state.selected_setting_detail != nullptr) {
      state.selected_setting_detail->clear();
      *state.updated_ui = true;
    }
  }

  if (timeout_expired_once(now, state.last_climate_target_focus_interaction, 5000)) {
    *state.climate_target_focus = 0;
    *state.climate_target_focus_value = NAN;
    *state.updated_ui = true;
  }
}

}  // namespace esphome
