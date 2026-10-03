#include "remote_ui_sync.h"

#include <ctime>

#include "esphome/core/time.h"

namespace esphome {

static inline bool assign_string_if_changed(std::string *target, const std::string &value) {
  if (*target != value) {
    *target = value;
    return true;
  }
  return false;
}

static inline bool assign_cstr_if_changed(std::string *target, const char *value) {
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
  bool target_nan = std::isnan(*target);
  bool value_nan = std::isnan(value);
  if ((target_nan && value_nan) || (!target_nan && !value_nan && *target == value)) {
    return false;
  }
  *target = value;
  return true;
}

// The trackers hold "" until Home Assistant has sent a state, which leaves the
// screen on SYNCING. Anything else, "unknown" included, is what Home Assistant
// reports and goes on screen.
static inline void sync_simple_state(RemoteUiSyncState &ui, const std::string &state) {
  if (state.empty()) {
    return;
  }
  *ui.updated_ui = assign_string_if_changed(ui.selected_item_state, state) || *ui.updated_ui;
}

// Shared LIGHTS/FANS sync: an on/off state plus an optional percentage.
// zero_when_missing: a missing value means none at all (a fan without speeds,
// a light that can't dim), so the screen says ON; otherwise it hasn't arrived
// yet and the light is assumed to be at full brightness.
static inline void sync_toggle_percent_mode(
    RemoteUiSyncState &ui, const std::string &state, bool has_value, float value, float scale,
    int *pct_field, bool zero_when_missing) {
  if (state.empty()) {
    return;
  }
  int next_pct = *pct_field;

  if (state == "on") {
    // A reported value of 0 is a real reading, not a missing one: only fall back
    // to the assumed-100 path when the value is genuinely unavailable.
    if (has_value && !std::isnan(value)) {
      next_pct = clamp_percent_value(value, scale, value > 0 ? 1 : 0);
    } else if (zero_when_missing) {
      if (!has_value) {
        next_pct = 0;
      }
    } else if (next_pct <= 0) {
      next_pct = 100;
    }
  } else if (state == "off") {
    next_pct = 0;
  }

  if (*ui.selected_item_state != state || next_pct != *pct_field) {
    *ui.selected_item_state = state;
    *pct_field = next_pct;
    *ui.updated_ui = true;
  }
}

// A timestamp sensor's state in local time: "5:30 PM" today, otherwise
// "Oct 2, 5:30 PM"; a date sensor's "Oct 2". Home Assistant reports both in
// ISO 8601, a timestamp in UTC, which is long and hard to read on the panel.
// Any other state comes back unchanged.
static const std::string &sensor_display_state(const std::string &state, std::string &formatted) {
  int64_t epoch = 0;
  bool date_only = false;
  if (!parse_ha_timestamp(state, &epoch, &date_only)) {
    return state;
  }
  static const char *const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char buffer[32];
  if (date_only) {
    int year = 0, month = 0, day = 0;
    sscanf(state.c_str(), "%4d-%2d-%2d", &year, &month, &day);
    snprintf(buffer, sizeof(buffer), "%s %d, %d", MONTHS[month - 1], day, year);
    formatted = buffer;
    return formatted;
  }
  ESPTime local = ESPTime::from_epoch_local(static_cast<time_t>(epoch));
  ESPTime now = ESPTime::from_epoch_local(::time(nullptr));
  int hour = local.hour % 12;
  if (hour == 0) {
    hour = 12;
  }
  const char *meridiem = local.hour >= 12 ? "PM" : "AM";
  if (now.is_valid() && now.year == local.year && now.day_of_year == local.day_of_year) {
    snprintf(buffer, sizeof(buffer), "%d:%02d %s", hour, local.minute, meridiem);
  } else if (local.month >= 1 && local.month <= 12) {
    snprintf(buffer, sizeof(buffer), "%s %d, %d:%02d %s", MONTHS[local.month - 1], local.day_of_month, hour,
             local.minute, meridiem);
  } else {
    return state;
  }
  formatted = buffer;
  return formatted;
}

// Whether Home Assistant reports the entity behind this screen as unavailable.
static bool tracked_entity_unavailable(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS: return selected_light_state(idx) == "unavailable";
    case REMOTE_MODE_FANS: return selected_fan_state(idx) == "unavailable";
    case REMOTE_MODE_HUMIDIFIERS: return selected_humidifier_state(idx) == "unavailable";
    case REMOTE_MODE_SWITCHES: return selected_switch_state(idx) == "unavailable";
    case REMOTE_MODE_CLIMATE: return selected_climate_state(idx) == "unavailable";
    case REMOTE_MODE_WATER_HEATERS: return selected_water_heater_state(idx) == "unavailable";
    case REMOTE_MODE_LOCKS: return selected_lock_state(idx) == "unavailable";
    case REMOTE_MODE_COVERS: return selected_cover_state(idx) == "unavailable";
    case REMOTE_MODE_MEDIA: return selected_media_state(idx) == "unavailable";
    case REMOTE_MODE_SENSORS: return sensor_state_for_index(idx) == "unavailable";
    case REMOTE_MODE_AUTOMATION: return automation_state_for_index(idx) == "unavailable";
    case REMOTE_MODE_ALARMS: return alarm_state_for_index(idx) == "unavailable";
    case REMOTE_MODE_WEATHER: return weather_state_for_index(idx) == "unavailable";
    default: return false;
  }
}

void sync_remote_ui_state(RemoteMode mode, int idx, RemoteUiSyncState &ui) {
  // An unavailable entity says so, rather than SYNCING (no state is on its way)
  // or the last state it reported (which no longer holds).
  if (tracked_entity_unavailable(mode, idx)) {
    *ui.updated_ui = assign_cstr_if_changed(ui.selected_item_state, "unavailable") || *ui.updated_ui;
    if (mode == REMOTE_MODE_WEATHER) {
      // The weather screen goes by its condition.
      *ui.updated_ui = assign_cstr_if_changed(ui.selected_weather_condition, "unavailable") || *ui.updated_ui;
    }
    return;
  }

  if (mode == REMOTE_MODE_LIGHTS) {
    // A light that can't dim never reports a brightness: it shows ON.
    sync_toggle_percent_mode(ui, selected_light_state(idx), selected_light_has_brightness(idx),
                             selected_light_brightness(idx), 100.0f / 255.0f, ui.selected_brightness_pct,
                             selected_light_dimmable(idx) == 0);
    return;
  }

  if (mode == REMOTE_MODE_FANS) {
    sync_toggle_percent_mode(ui, selected_fan_state(idx), selected_fan_has_percentage(idx),
                             selected_fan_percentage(idx), 1.0f, ui.selected_fan_speed_pct, true);
    return;
  }

  if (mode == REMOTE_MODE_HUMIDIFIERS) {
    const std::string &state = selected_humidifier_state(idx);
    const std::string &mode_value = humidifier_mode_for_index(idx);
    const std::string &action = humidifier_action_for_index(idx);
    float target = humidifier_target_humidity_for_index(idx);
    float current = humidifier_current_humidity_for_index(idx);
    bool changed = false;

    if (!state.empty() || !std::isnan(target) || !std::isnan(current)) {
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
    sync_simple_state(ui, selected_switch_state(idx));
    return;
  }

  if (mode == REMOTE_MODE_CLIMATE) {
    const std::string &state = selected_climate_state(idx);
    const std::string &hvac_action = climate_hvac_action_for_index(idx);
    const std::string &fan_mode = climate_fan_mode_for_index(idx);
    const std::string &preset = selected_climate_preset_mode(idx);
    float target = selected_climate_target_temperature(idx);
    float target_low = selected_climate_target_temperature_low(idx);
    float target_high = selected_climate_target_temperature_high(idx);
    float current = selected_climate_current_temperature(idx);
    float humidity = climate_target_humidity_for_index(idx);
    bool changed = false;

    // A thermostat whose mode Home Assistant reports as unknown still has
    // temperatures worth showing.
    if (!state.empty() || !std::isnan(current) || !std::isnan(target)) {
      changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      if (!hvac_action.empty()) {
        changed = assign_string_if_changed(ui.selected_climate_hvac_action, hvac_action) || changed;
      }
      changed = assign_string_if_changed(ui.selected_climate_fan_mode, fan_mode) || changed;
      changed = assign_string_if_changed(ui.selected_climate_preset, preset) || changed;
      changed = assign_float_if_changed(ui.selected_climate_target_temp, target) || changed;
      changed = assign_float_if_changed(ui.selected_climate_target_temp_low, target_low) || changed;
      changed = assign_float_if_changed(ui.selected_climate_target_temp_high, target_high) || changed;
      changed = assign_float_if_changed(ui.selected_climate_current_temp, current) || changed;
      changed = assign_float_if_changed(ui.selected_climate_target_humidity, humidity) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_WATER_HEATERS) {
    const std::string &state = selected_water_heater_state(idx);
    const std::string &operation_mode = selected_water_heater_operation_mode(idx);
    const std::string &away_mode = selected_water_heater_away_mode(idx);
    float target = selected_water_heater_target_temperature(idx);
    bool changed = false;
    if (!state.empty() || !std::isnan(target)) {
      changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      changed = assign_string_if_changed(ui.selected_water_heater_mode, operation_mode) || changed;
      changed = assign_string_if_changed(ui.selected_water_heater_away, away_mode) || changed;
      changed = assign_float_if_changed(ui.selected_water_heater_target_temp, target) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_LOCKS) {
    sync_simple_state(ui, selected_lock_state(idx));
    return;
  }

  if (mode == REMOTE_MODE_COVERS) {
    const std::string &state = selected_cover_state(idx);
    float position = selected_cover_position(idx);
    bool changed = false;

    if (!state.empty() || !std::isnan(position)) {
      if (!state.empty()) {
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
    const std::string &shuffle = media_shuffle_for_index(idx);
    const std::string &repeat = media_repeat_for_index(idx);
    const std::string &sound_mode = media_sound_mode_for_index(idx);
    float volume = selected_media_volume(idx);
    bool changed = false;

    if (!state.empty()) {
      changed = assign_string_if_changed(ui.selected_item_state, state) || changed;
      changed = assign_string_if_changed(ui.selected_media_title, title) || changed;
      changed = assign_string_if_changed(ui.selected_media_artist, artist) || changed;
      changed = assign_string_if_changed(ui.selected_media_device_class, device_class) || changed;
      changed = assign_string_if_changed(ui.selected_media_shuffle, shuffle) || changed;
      changed = assign_string_if_changed(ui.selected_media_repeat, repeat) || changed;
      changed = assign_string_if_changed(ui.selected_media_sound_mode, sound_mode) || changed;
      if (device_class == "tv" || device_class == "receiver") {
        if (!source.empty()) {
          changed = assign_string_if_changed(ui.selected_media_source, source) || changed;
        }
      } else {
        changed = assign_string_if_changed(ui.selected_media_source, source) || changed;
      }
      // -1 while the player reports no volume, so the remote never steps from
      // another player's level.
      int volume_pct = std::isnan(volume) ? -1 : clamp_percent_value(volume, 100.0f);
      changed = assign_int_if_changed(ui.selected_media_volume_pct, volume_pct) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_SENSORS) {
    bool changed = false;
    const std::string &state = sensor_state_for_index(idx);
    const std::string &unit = sensor_unit_for_index(idx);
    if (!state.empty()) {
      std::string formatted;
      changed = assign_string_if_changed(ui.selected_item_state, sensor_display_state(state, formatted)) || changed;
      changed = assign_string_if_changed(ui.selected_sensor_unit, unit) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_AUTOMATION) {
    const std::string &state = automation_state_for_index(idx);
    // Automations report on/off for enabled/disabled and scripts for
    // running/idle; a scene's state is only the time it last ran ("unknown"
    // if it never has), so any state means it is ready.
    std::string next_state =
        automation_kind(idx) == AUTOMATION_KIND_SCENE && !state.empty() ? std::string("ready") : state;
    bool changed = false;
    changed = assign_string_if_changed(ui.selected_item_state, next_state) || changed;
    if (changed) *ui.updated_ui = true;
    return;
  }

  if (mode == REMOTE_MODE_ALARMS) {
    sync_simple_state(ui, alarm_state_for_index(idx));
    return;
  }

  if (mode == REMOTE_MODE_NOTIFICATIONS) {
    int count = notification_mode_item_count();
    int clamped_idx = clamp_mode_index(idx, count);
    char next_name[24];
    notification_mode_item_label(clamped_idx, next_name, sizeof(next_name));
    const char *next_entity = notification_mode_item_entity_cstr();
    const std::string &next_message = notification_message_for_index(clamped_idx);

    if (assign_int_if_changed(ui.selected_notification_index, clamped_idx)) {
      *ui.updated_ui = true;
    }
    *ui.updated_ui = assign_cstr_if_changed(ui.selected_item_name, next_name) || *ui.updated_ui;
    *ui.updated_ui = assign_cstr_if_changed(ui.selected_item_entity, next_entity) || *ui.updated_ui;
    *ui.updated_ui = assign_string_if_changed(ui.selected_item_state, next_message) || *ui.updated_ui;
    return;
  }

  if (mode == REMOTE_MODE_WEATHER) {
    const std::string &condition = weather_state_for_index(idx);
    float temperature = weather_temperature_for_index(idx);
    float humidity = weather_humidity_for_index(idx);
    float high = weather_high_temperature_for_index(idx);
    float low = weather_low_temperature_for_index(idx);
    float wind_speed = weather_wind_speed_for_index(idx);
    float wind_bearing = weather_wind_bearing_for_index(idx);
    float wind_gust_speed = weather_wind_gust_speed_for_index(idx);
    float pressure = weather_pressure_for_index(idx);
    float cloud_coverage = weather_cloud_coverage_for_index(idx);
    float uv_index = weather_uv_index_for_index(idx);
    float dew_point = weather_dew_point_for_index(idx);
    float apparent_temperature = weather_apparent_temperature_for_index(idx);
    float precipitation = weather_precipitation_for_index(idx);
    bool changed = false;

    if (!condition.empty() || !std::isnan(temperature) || !std::isnan(humidity) || !std::isnan(high) ||
        !std::isnan(low)) {
      changed = assign_string_if_changed(ui.selected_item_state, condition) || changed;
      changed = assign_string_if_changed(ui.selected_weather_condition, condition) || changed;
      changed = assign_float_if_changed(ui.selected_weather_temperature, temperature) || changed;
      changed = assign_float_if_changed(ui.selected_weather_humidity, humidity) || changed;
      changed = assign_float_if_changed(ui.selected_weather_high_temp, high) || changed;
      changed = assign_float_if_changed(ui.selected_weather_low_temp, low) || changed;
      changed = assign_float_if_changed(ui.selected_weather_wind_speed, wind_speed) || changed;
      changed = assign_float_if_changed(ui.selected_weather_wind_bearing, wind_bearing) || changed;
      changed = assign_float_if_changed(ui.selected_weather_wind_gust_speed, wind_gust_speed) || changed;
      changed = assign_float_if_changed(ui.selected_weather_pressure, pressure) || changed;
      changed = assign_float_if_changed(ui.selected_weather_cloud_coverage, cloud_coverage) || changed;
      changed = assign_float_if_changed(ui.selected_weather_uv_index, uv_index) || changed;
      changed = assign_float_if_changed(ui.selected_weather_dew_point, dew_point) || changed;
      changed = assign_float_if_changed(ui.selected_weather_apparent_temperature, apparent_temperature) || changed;
      changed = assign_float_if_changed(ui.selected_weather_precipitation, precipitation) || changed;
      if (changed) *ui.updated_ui = true;
    }
    return;
  }

  if (mode == REMOTE_MODE_INFO) {
    const char *state =
        idx == 0 ? "date"
                 : idx == 1 ? "wireless"
                 : idx == 2 ? "network"
                 : idx == 3 ? "device_name"
                 : idx == 4 ? "battery"
                            : "version";
    *ui.updated_ui = assign_cstr_if_changed(ui.selected_item_state, state) || *ui.updated_ui;
  }
}

}  // namespace esphome
