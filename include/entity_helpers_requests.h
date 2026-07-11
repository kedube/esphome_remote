#pragma once

#include "entity_trackers.h"

inline LightStatusTracker light_status_tracker_storage;
inline SwitchStatusTracker switch_status_tracker_storage(SWITCH_LIST);
inline FanStatusTracker fan_status_tracker_storage;
inline HumidifierStatusTracker humidifier_status_tracker_storage;
inline ClimateStatusTracker climate_status_tracker_storage;
inline WaterHeaterStatusTracker water_heater_status_tracker_storage;
inline LockStatusTracker lock_status_tracker_storage(LOCK_LIST);
inline CoverStatusTracker cover_status_tracker_storage;
inline MediaStatusTracker media_status_tracker_storage;
inline SensorStatusTracker sensor_status_tracker_storage(SENSOR_LIST);
inline AutomationStatusTracker automation_status_tracker_storage(AUTOMATION_LIST);
inline AlarmStatusTracker alarm_status_tracker_storage(ALARM_LIST);
inline NotificationFeedTracker notification_feed_tracker_storage;
inline WeatherStatusTracker weather_status_tracker_storage;
inline bool remote_status_trackers_initialized = false;

// Sets up all trackers once. State is delivered exclusively through the
// subscriptions registered here: Home Assistant pushes the current value of
// every subscribed state/attribute right after the API handshake (and again on
// every reconnect), then streams changes. The request_selected_*_status
// helpers below therefore only need to guarantee this setup ran — issuing
// explicit fetches via get_home_assistant_state() would permanently grow the
// API server's subscription vector without ever being announced to Home
// Assistant once the handshake is done.
inline void ensure_remote_status_trackers() {
  if (remote_status_trackers_initialized) {
    return;
  }
  if (!ha_api_ready()) {
    // The API server component has not been set up yet; retry on the next
    // call instead of dereferencing a null global_api_server.
    return;
  }
  validate_remote_configuration();
  light_status_tracker_storage.setup();
  switch_status_tracker_storage.setup();
  fan_status_tracker_storage.setup();
  humidifier_status_tracker_storage.setup();
  climate_status_tracker_storage.setup();
  water_heater_status_tracker_storage.setup();
  lock_status_tracker_storage.setup();
  cover_status_tracker_storage.setup();
  media_status_tracker_storage.setup();
  sensor_status_tracker_storage.setup();
  automation_status_tracker_storage.setup();
  alarm_status_tracker_storage.setup();
  notification_feed_tracker_storage.setup();
  weather_status_tracker_storage.setup();
  remote_status_trackers_initialized = true;
}

inline void ensure_light_status_tracker() { ensure_remote_status_trackers(); }

inline void request_selected_light_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_light_state(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.state(idx);
}

inline bool selected_light_has_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.has_brightness(idx);
}

inline float selected_light_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.brightness(idx);
}

inline bool selected_light_has_effect(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.has_effect(idx);
}

inline const std::string &selected_light_effect(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.effect(idx);
}

inline const std::string &selected_light_effect_list(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.effect_list(idx);
}

inline std::string next_light_effect_for_index(int idx) {
  return next_delimited_option(selected_light_effect_list(idx), selected_light_effect(idx));
}

inline std::string previous_light_effect_for_index(int idx) {
  return previous_delimited_option(selected_light_effect_list(idx), selected_light_effect(idx));
}

inline void request_selected_switch_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_switch_state(int idx) {
  ensure_remote_status_trackers();
  return switch_status_tracker_storage.state(idx);
}

inline void request_selected_fan_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_fan_state(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.state(idx);
}

inline bool selected_fan_has_percentage(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.has_percentage(idx);
}

inline float selected_fan_percentage(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.percentage(idx);
}

inline const std::string &fan_preset_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.preset_mode(idx);
}

inline const std::string &fan_preset_modes_for_index(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.preset_modes(idx);
}

inline const std::string &fan_oscillating_for_index(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.oscillating(idx);
}

inline const std::string &fan_direction_for_index(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.direction(idx);
}

inline std::string next_fan_preset_for_index(int idx) {
  return next_delimited_option(fan_preset_modes_for_index(idx), fan_preset_mode_for_index(idx));
}

inline std::string previous_fan_preset_for_index(int idx) {
  return previous_delimited_option(fan_preset_modes_for_index(idx), fan_preset_mode_for_index(idx));
}

inline void request_selected_humidifier_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_humidifier_state(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.state(idx);
}

inline const std::string &humidifier_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.action(idx);
}

inline const std::string &humidifier_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.mode(idx);
}

inline const std::string &humidifier_available_modes_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.available_modes(idx);
}

inline std::string next_humidifier_mode_for_index(int idx) {
  return next_delimited_option(humidifier_available_modes_for_index(idx), humidifier_mode_for_index(idx));
}

inline std::string previous_humidifier_mode_for_index(int idx) {
  return previous_delimited_option(humidifier_available_modes_for_index(idx), humidifier_mode_for_index(idx));
}

inline float humidifier_target_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.target_humidity(idx);
}

inline float humidifier_current_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.current_humidity(idx);
}

inline void request_selected_climate_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline void request_selected_water_heater_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_climate_state(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.state(idx);
}

inline const std::string &climate_hvac_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.hvac_action(idx);
}

inline const std::string &selected_climate_preset_mode(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.preset_mode(idx);
}

inline bool climate_supports_preset(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.supports_preset(idx);
}

inline const std::string &climate_hvac_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.hvac_mode(idx);
}

inline const std::string &selected_climate_hvac_modes(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.hvac_modes(idx);
}

inline const std::string &climate_fan_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.fan_mode(idx);
}

inline const std::string &selected_climate_fan_modes(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.fan_modes(idx);
}

inline float climate_target_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_humidity(idx);
}

inline const std::string &selected_climate_preset_modes(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.preset_modes(idx);
}

inline std::string next_climate_hvac_mode_for_index(int idx) {
  return next_delimited_option(selected_climate_hvac_modes(idx), climate_hvac_mode_for_index(idx));
}

inline std::string previous_climate_hvac_mode_for_index(int idx) {
  return previous_delimited_option(selected_climate_hvac_modes(idx), climate_hvac_mode_for_index(idx));
}

inline std::string next_climate_fan_mode_for_index(int idx) {
  return next_delimited_option(selected_climate_fan_modes(idx), climate_fan_mode_for_index(idx));
}

inline std::string previous_climate_fan_mode_for_index(int idx) {
  return previous_delimited_option(selected_climate_fan_modes(idx), climate_fan_mode_for_index(idx));
}

inline std::string next_climate_preset_for_index(int idx) {
  return next_delimited_option(selected_climate_preset_modes(idx), selected_climate_preset_mode(idx));
}

inline std::string previous_climate_preset_for_index(int idx) {
  return previous_delimited_option(selected_climate_preset_modes(idx), selected_climate_preset_mode(idx));
}

inline float selected_climate_target_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature(idx);
}

inline float selected_climate_target_temperature_low(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_low(idx);
}

inline float selected_climate_target_temperature_high(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_high(idx);
}

inline float selected_climate_current_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.current_temperature(idx);
}

inline const std::string &selected_water_heater_state(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.state(idx);
}

inline float selected_water_heater_target_temperature(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.target_temperature(idx);
}

inline const std::string &selected_water_heater_operation_mode(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.operation_mode(idx);
}

inline const std::string &selected_water_heater_operation_list(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.operation_list(idx);
}

inline const std::string &selected_water_heater_away_mode(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.away_mode(idx);
}

inline std::string next_water_heater_operation_mode_for_index(int idx) {
  return next_delimited_option(selected_water_heater_operation_list(idx), selected_water_heater_operation_mode(idx));
}

inline std::string previous_water_heater_operation_mode_for_index(int idx) {
  return previous_delimited_option(selected_water_heater_operation_list(idx), selected_water_heater_operation_mode(idx));
}

inline void request_selected_lock_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_lock_state(int idx) {
  ensure_remote_status_trackers();
  return lock_status_tracker_storage.state(idx);
}

inline void request_selected_cover_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_cover_state(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.state(idx);
}

inline float selected_cover_position(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.position(idx);
}

inline bool selected_cover_has_position(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.has_position(idx);
}

inline bool selected_cover_has_tilt(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.has_tilt(idx);
}

inline float selected_cover_tilt(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.tilt(idx);
}

inline void request_selected_media_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &selected_media_state(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.state(idx);
}

inline const std::string &media_title_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.title(idx);
}

inline const std::string &media_device_class_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.device_class(idx);
}

inline const std::string &media_source_list_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source_list(idx);
}

inline void set_media_source_for_index(int idx, const std::string &source) {
  ensure_remote_status_trackers();
  media_status_tracker_storage.set_source(idx, source);
}

inline const std::string &media_artist_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.artist(idx);
}

inline const std::string &media_source_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source(idx);
}

inline float selected_media_volume(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.volume(idx);
}

inline const std::string &media_shuffle_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.shuffle(idx);
}

inline const std::string &media_repeat_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.repeat(idx);
}

inline const std::string &media_sound_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.sound_mode(idx);
}

inline const std::string &selected_media_sound_mode_list(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.sound_mode_list(idx);
}

inline std::string next_media_sound_mode_for_index(int idx) {
  return next_delimited_option(selected_media_sound_mode_list(idx), media_sound_mode_for_index(idx));
}

inline std::string previous_media_sound_mode_for_index(int idx) {
  return previous_delimited_option(selected_media_sound_mode_list(idx), media_sound_mode_for_index(idx));
}

inline void request_selected_sensor_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &sensor_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.state(idx);
}

inline const std::string &sensor_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.unit(idx);
}

inline void request_selected_automation_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &automation_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.state(idx);
}

inline void request_selected_alarm_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &alarm_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return alarm_status_tracker_storage.state(idx);
}

inline void request_selected_notification_status(int idx) {
  (void) idx;
  if (!notifications_mode_enabled()) {
    return;
  }
  ensure_remote_status_trackers();
}

inline int notification_mode_item_count() {
  if (!notifications_mode_enabled()) {
    return 0;
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.item_count();
}

inline std::string notification_mode_item_name(int idx) {
  if (!notifications_mode_enabled()) {
    return "";
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.label(idx);
}

inline std::string notification_mode_item_entity(int idx) {
  (void) idx;
  if (!notifications_mode_enabled()) {
    return "";
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.entity();
}

// Allocation-free variants for the periodic sync path.
inline void notification_mode_item_label(int idx, char *buffer, size_t buffer_size) {
  if (buffer == nullptr || buffer_size == 0) {
    return;
  }
  buffer[0] = '\0';
  if (!notifications_mode_enabled()) {
    return;
  }
  ensure_remote_status_trackers();
  notification_feed_tracker_storage.label_to_buffer(idx, buffer, buffer_size);
}

inline const char *notification_mode_item_entity_cstr() {
  return notifications_mode_enabled() ? NotificationFeedTracker::entity_cstr() : "";
}

inline const std::string &notification_message_for_index(int idx) {
  if (!notifications_mode_enabled()) {
    return empty_string();
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.message(idx);
}

inline const std::string &notification_id_for_index(int idx) {
  if (!notifications_mode_enabled()) {
    return empty_string();
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.notification_id(idx);
}

inline void request_selected_weather_status(int idx) {
  (void) idx;
  ensure_remote_status_trackers();
}

inline const std::string &weather_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.state(idx);
}

inline float weather_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.temperature(idx);
}

inline float weather_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.humidity(idx);
}

inline float weather_high_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.high_temperature(idx);
}

inline float weather_low_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.low_temperature(idx);
}

inline float weather_wind_speed_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.wind_speed(idx);
}

inline float weather_wind_bearing_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.wind_bearing(idx);
}

inline float weather_wind_gust_speed_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.wind_gust_speed(idx);
}

inline float weather_pressure_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.pressure(idx);
}

inline float weather_cloud_coverage_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.cloud_coverage(idx);
}

inline float weather_uv_index_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.uv_index(idx);
}

inline float weather_dew_point_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.dew_point(idx);
}

inline float weather_apparent_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.apparent_temperature(idx);
}

inline float weather_precipitation_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.precipitation(idx);
}

inline void request_mode_status(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      request_selected_light_status(idx);
      break;
    case REMOTE_MODE_SWITCHES:
      request_selected_switch_status(idx);
      break;
    case REMOTE_MODE_FANS:
      request_selected_fan_status(idx);
      break;
    case REMOTE_MODE_HUMIDIFIERS:
      request_selected_humidifier_status(idx);
      break;
    case REMOTE_MODE_CLIMATE:
      request_selected_climate_status(idx);
      break;
    case REMOTE_MODE_WATER_HEATERS:
      request_selected_water_heater_status(idx);
      break;
    case REMOTE_MODE_LOCKS:
      request_selected_lock_status(idx);
      break;
    case REMOTE_MODE_COVERS:
      request_selected_cover_status(idx);
      break;
    case REMOTE_MODE_MEDIA:
      request_selected_media_status(idx);
      break;
    case REMOTE_MODE_SENSORS:
      request_selected_sensor_status(idx);
      break;
    case REMOTE_MODE_AUTOMATION:
      request_selected_automation_status(idx);
      break;
    case REMOTE_MODE_ALARMS:
      request_selected_alarm_status(idx);
      break;
    case REMOTE_MODE_NOTIFICATIONS:
      request_selected_notification_status(idx);
      break;
    case REMOTE_MODE_WEATHER:
      request_selected_weather_status(idx);
      break;
    case REMOTE_MODE_INFO:
      break;
  }
}

inline void request_mode_status_throttled(
    RemoteMode mode, int idx, uint32_t now, uint32_t min_interval_ms, bool force = false) {
  (void) now;
  (void) min_interval_ms;
  (void) force;
  // Requests are subscription-backed no-ops now, so no throttling is needed.
  request_mode_status(mode, idx);
}

inline uint32_t refresh_retry_interval_ms(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_CLIMATE:
    case REMOTE_MODE_MEDIA:
    case REMOTE_MODE_WEATHER:
      return 10000;
    case REMOTE_MODE_HUMIDIFIERS:
    case REMOTE_MODE_LIGHTS:
    case REMOTE_MODE_FANS:
      return 5000;
    default:
      return 2500;
  }
}
