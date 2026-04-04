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
  *state.selected_climate_hvac_action = "unknown";
  state.selected_climate_preset->clear();
  *state.selected_climate_target_temp_low = NAN;
  *state.selected_climate_target_temp_high = NAN;
  *state.selected_climate_current_temp = NAN;
  *state.climate_target_focus = 0;
  *state.climate_target_focus_value = NAN;
  *state.last_climate_target_focus_interaction = 0;
  state.selected_media_title->clear();
  state.selected_media_artist->clear();
  state.selected_media_device_class->clear();
  state.selected_media_source->clear();
  state.last_media_power_feedback->clear();
  *state.selected_automation_state = "unknown";
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
  *state.selected_weather_condition = "unknown";
  *state.updated_ui = true;
}

static inline bool timeout_window_hit(uint32_t now, uint32_t last_interaction, uint32_t start_ms, uint32_t end_ms) {
  if (last_interaction == 0) {
    return false;
  }
  uint32_t elapsed = now - last_interaction;
  return elapsed > start_ms && elapsed < end_ms;
}

void apply_remote_ui_timeout_updates(uint32_t now, RemoteUiTimeoutState &state) {
  if (timeout_window_hit(now, state.last_brightness_interaction, 3000, 3500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_switch_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_fan_speed_interaction, 3000, 3500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_humidifier_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_humidifier_mode_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_climate_interaction, 3000, 3500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_lock_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_cover_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_cover_position_interaction, 3000, 3500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_media_volume_interaction, 3000, 3500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_media_source_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_media_power_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_automation_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_alarm_interaction, 5000, 5500)) {
    state.updated_ui = true;
  }
  if (timeout_window_hit(now, state.last_contrast_interaction, 3000, 3500)) {
    state.updated_ui = true;
  }

  if (timeout_window_hit(now, state.last_climate_target_focus_interaction, 5000, 5500)) {
    state.climate_target_focus = 0;
    state.climate_target_focus_value = NAN;
    state.last_climate_target_focus_interaction = 0;
    state.updated_ui = true;
  }
}

}  // namespace esphome
