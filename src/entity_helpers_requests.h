#pragma once

static LightStatusTracker light_status_tracker_storage;
static SwitchStatusTracker switch_status_tracker_storage(SWITCH_LIST);
static FanStatusTracker fan_status_tracker_storage;
static HumidifierStatusTracker humidifier_status_tracker_storage;
static ClimateStatusTracker climate_status_tracker_storage;
static LockStatusTracker lock_status_tracker_storage(LOCK_LIST);
static CoverStatusTracker cover_status_tracker_storage;
static MediaStatusTracker media_status_tracker_storage;
static SensorStatusTracker sensor_status_tracker_storage(SENSOR_LIST);
static AutomationStatusTracker automation_status_tracker_storage(AUTOMATION_LIST);
static AlarmStatusTracker alarm_status_tracker_storage(ALARM_LIST);
static NotificationFeedTracker notification_feed_tracker_storage;
static WeatherStatusTracker weather_status_tracker_storage;
static bool remote_status_trackers_initialized = false;
static RemoteMode last_requested_mode = REMOTE_MODE_INFO;
static int last_requested_mode_index = -1;
static uint32_t last_requested_mode_at = 0;

static inline void ensure_remote_status_trackers() {
  if (remote_status_trackers_initialized) {
    return;
  }
  light_status_tracker_storage.setup();
  switch_status_tracker_storage.setup();
  fan_status_tracker_storage.setup();
  humidifier_status_tracker_storage.setup();
  climate_status_tracker_storage.setup();
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

static inline void ensure_light_status_tracker() { ensure_remote_status_trackers(); }

static inline void request_selected_light_status(int idx) {
  ensure_remote_status_trackers();
  light_status_tracker_storage.request_light_state(idx);
}

static inline const std::string &selected_light_state(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.state(idx);
}

static inline bool selected_light_has_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.has_brightness(idx);
}

static inline float selected_light_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.brightness(idx);
}

static inline void request_selected_switch_status(int idx) {
  ensure_remote_status_trackers();
  switch_status_tracker_storage.request_state(idx);
}

static inline const std::string &selected_switch_state(int idx) {
  ensure_remote_status_trackers();
  return switch_status_tracker_storage.state(idx);
}

static inline void request_selected_fan_status(int idx) {
  ensure_remote_status_trackers();
  fan_status_tracker_storage.request_fan_state(idx);
}

static inline const std::string &selected_fan_state(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.state(idx);
}

static inline bool selected_fan_has_percentage(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.has_percentage(idx);
}

static inline float selected_fan_percentage(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.percentage(idx);
}

static inline void request_selected_humidifier_status(int idx) {
  ensure_remote_status_trackers();
  humidifier_status_tracker_storage.request_humidifier_state(idx);
}

static inline const std::string &selected_humidifier_state(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.state(idx);
}

static inline const std::string &humidifier_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.action(idx);
}

static inline const std::string &humidifier_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.mode(idx);
}

static inline const std::string &humidifier_available_modes_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.available_modes(idx);
}

static inline std::string next_humidifier_mode_for_index(int idx) {
  std::string current_mode = trim_copy(humidifier_mode_for_index(idx));
  std::string available_modes = humidifier_available_modes_for_index(idx);
  int count = delimited_option_count(available_modes);
  if (count <= 0) {
    return "";
  }
  for (int i = 0; i < count; i++) {
    if (delimited_option_at(available_modes, i) == current_mode) {
      return delimited_option_at(available_modes, (i + 1) % count);
    }
  }
  return delimited_option_at(available_modes, 0);
}

static inline float humidifier_target_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.target_humidity(idx);
}

static inline float humidifier_current_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.current_humidity(idx);
}

static inline void request_selected_climate_status(int idx) {
  ensure_remote_status_trackers();
  climate_status_tracker_storage.request_climate_state(idx);
}

static inline const std::string &selected_climate_state(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.state(idx);
}

static inline const std::string &climate_hvac_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.hvac_action(idx);
}

static inline const std::string &selected_climate_preset_mode(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.preset_mode(idx);
}

static inline bool climate_supports_preset(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.supports_preset(idx);
}

static inline float selected_climate_target_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature(idx);
}

static inline float selected_climate_target_temperature_low(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_low(idx);
}

static inline float selected_climate_target_temperature_high(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_high(idx);
}

static inline float selected_climate_current_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.current_temperature(idx);
}

static inline void request_selected_lock_status(int idx) {
  ensure_remote_status_trackers();
  lock_status_tracker_storage.request_state(idx);
}

static inline const std::string &selected_lock_state(int idx) {
  ensure_remote_status_trackers();
  return lock_status_tracker_storage.state(idx);
}

static inline void request_selected_cover_status(int idx) {
  ensure_remote_status_trackers();
  cover_status_tracker_storage.request_cover_state(idx);
}

static inline const std::string &selected_cover_state(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.state(idx);
}

static inline float selected_cover_position(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.position(idx);
}

static inline void request_selected_media_status(int idx) {
  ensure_remote_status_trackers();
  media_status_tracker_storage.request_media_state(idx);
}

static inline const std::string &selected_media_state(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.state(idx);
}

static inline const std::string &media_title_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.title(idx);
}

static inline const std::string &media_device_class_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.device_class(idx);
}

static inline const std::string &media_source_list_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source_list(idx);
}

static inline void set_media_source_for_index(int idx, const std::string &source) {
  ensure_remote_status_trackers();
  media_status_tracker_storage.set_source(idx, source);
}

static inline const std::string &media_artist_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.artist(idx);
}

static inline const std::string &media_source_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source(idx);
}

static inline float selected_media_volume(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.volume(idx);
}

static inline void request_selected_sensor_status(int idx) {
  ensure_remote_status_trackers();
  sensor_status_tracker_storage.request_state(idx);
}

static inline const std::string &sensor_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.state(idx);
}

static inline void request_selected_automation_status(int idx) {
  ensure_remote_status_trackers();
  automation_status_tracker_storage.request_state(idx);
}

static inline const std::string &automation_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.state(idx);
}

static inline void request_selected_alarm_status(int idx) {
  ensure_remote_status_trackers();
  alarm_status_tracker_storage.request_state(idx);
}

static inline const std::string &alarm_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return alarm_status_tracker_storage.state(idx);
}

static inline void request_selected_notification_status(int idx) {
  (void) idx;
  if (!notifications_mode_enabled()) {
    return;
  }
  ensure_remote_status_trackers();
  notification_feed_tracker_storage.request_notifications();
}

static inline int notification_mode_item_count() {
  if (!notifications_mode_enabled()) {
    return 0;
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.item_count();
}

static inline std::string notification_mode_item_name(int idx) {
  if (!notifications_mode_enabled()) {
    return "";
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.label(idx);
}

static inline std::string notification_mode_item_entity(int idx) {
  (void) idx;
  if (!notifications_mode_enabled()) {
    return "";
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.entity();
}

static inline const std::string &notification_message_for_index(int idx) {
  if (!notifications_mode_enabled()) {
    return empty_string();
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.message(idx);
}

static inline const std::string &notification_id_for_index(int idx) {
  if (!notifications_mode_enabled()) {
    return empty_string();
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.notification_id(idx);
}

static inline void request_selected_weather_status(int idx) {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.request_weather_state(idx);
}

static inline void request_all_weather_status() {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.request_all_states();
}

static inline const std::string &weather_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.state(idx);
}

static inline float weather_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.temperature(idx);
}

static inline float weather_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.humidity(idx);
}

static inline float weather_high_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.high_temperature(idx);
}

static inline float weather_low_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.low_temperature(idx);
}

static inline void request_mode_status(RemoteMode mode, int idx) {
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

static inline void request_mode_status_throttled(
    RemoteMode mode, int idx, uint32_t now, uint32_t min_interval_ms, bool force = false) {
  if (!force && last_requested_mode == mode && last_requested_mode_index == idx &&
      (now - last_requested_mode_at) < min_interval_ms) {
    return;
  }

  request_mode_status(mode, idx);
  last_requested_mode = mode;
  last_requested_mode_index = idx;
  last_requested_mode_at = now;
}
