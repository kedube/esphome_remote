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
inline SensorStatusTracker sensor_status_tracker_storage;
inline AutomationStatusTracker automation_status_tracker_storage(AUTOMATION_LIST);
inline AlarmStatusTracker alarm_status_tracker_storage(ALARM_LIST);
inline NotificationFeedTracker notification_feed_tracker_storage;
inline WeatherStatusTracker weather_status_tracker_storage;
inline bool remote_status_trackers_initialized = false;

// Order in which entities announce their subscriptions. Home Assistant answers
// each subscription with the current value as soon as it arrives, but the API
// server announces only one per main-loop pass, so with a few hundred
// subscriptions the last entity can take seconds to sync after every wake.
// Announcing the restored selection first, then the rest of its favorite list,
// fills the screen the remote wakes into before anything else.
struct TrackerSubscriptionOrder {
  static constexpr int RANK_COUNT = 3;

  const char *selected_entity_id = nullptr;
  int favorite_list_index = -1;

  int rank(const char *entity_id) const {
    if (this->selected_entity_id != nullptr && strcmp(entity_id, this->selected_entity_id) == 0) {
      return 0;
    }
    for (int i = 0; i < favorite_list_item_count(this->favorite_list_index); i++) {
      if (strcmp(entity_id, FAVORITE_LISTS[this->favorite_list_index].entries[i].entity_id) == 0) {
        return 1;
      }
    }
    return 2;
  }
};

inline TrackerSubscriptionOrder tracker_subscription_order_for_menu(int menu_index) {
  TrackerSubscriptionOrder order;
  if (menu_index_is_favorite(menu_index)) {
    order.favorite_list_index = menu_index;
    const FavoriteEntity *entry = favorite_list_entry(menu_index, favorite_selected_index_ref(menu_index));
    if (entry != nullptr) {
      order.selected_entity_id = entry->entity_id;
    }
  } else if (menu_index_is_notifications(menu_index)) {
    order.selected_entity_id = NotificationFeedTracker::entity_cstr();
  }
  return order;
}

template <typename Tracker>
inline void subscribe_tracker_rank(Tracker &tracker, const TrackerSubscriptionOrder &order, int rank) {
  for (int i = 0; i < Tracker::COUNT; i++) {
    if (order.rank(tracker.entity_id(i)) == rank) {
      tracker.subscribe(i);
    }
  }
}

// Sets up all trackers once. State is delivered exclusively through the
// subscriptions registered here: Home Assistant pushes the current value of
// every subscribed state/attribute right after the API handshake (and again on
// every reconnect), then streams changes, so nothing ever needs to "refresh"
// a tracker — issuing explicit fetches via get_home_assistant_state() would
// permanently grow the API server's subscription vector without ever being
// announced to Home Assistant once the handshake is done.
//
// on_boot calls this with the restored selection; the lazy calls from the
// accessors below are a fallback and use the default (declaration) order.
inline void ensure_remote_status_trackers(const TrackerSubscriptionOrder &order = {}) {
  if (remote_status_trackers_initialized) {
    return;
  }
  if (!ha_api_ready()) {
    // The API server component has not been set up yet; retry on the next
    // call instead of dereferencing a null global_api_server.
    return;
  }
  validate_remote_configuration();
  for (int rank = 0; rank < TrackerSubscriptionOrder::RANK_COUNT; rank++) {
    subscribe_tracker_rank(light_status_tracker_storage, order, rank);
    subscribe_tracker_rank(switch_status_tracker_storage, order, rank);
    subscribe_tracker_rank(fan_status_tracker_storage, order, rank);
    subscribe_tracker_rank(humidifier_status_tracker_storage, order, rank);
    subscribe_tracker_rank(climate_status_tracker_storage, order, rank);
    subscribe_tracker_rank(water_heater_status_tracker_storage, order, rank);
    subscribe_tracker_rank(lock_status_tracker_storage, order, rank);
    subscribe_tracker_rank(cover_status_tracker_storage, order, rank);
    subscribe_tracker_rank(media_status_tracker_storage, order, rank);
    subscribe_tracker_rank(sensor_status_tracker_storage, order, rank);
    subscribe_tracker_rank(automation_status_tracker_storage, order, rank);
    subscribe_tracker_rank(alarm_status_tracker_storage, order, rank);
    if (order.rank(NotificationFeedTracker::entity_cstr()) == rank) {
      notification_feed_tracker_storage.subscribe();
    }
    subscribe_tracker_rank(weather_status_tracker_storage, order, rank);
  }
  remote_status_trackers_initialized = true;
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

inline const std::string &selected_switch_state(int idx) {
  ensure_remote_status_trackers();
  return switch_status_tracker_storage.state(idx);
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

inline float humidifier_target_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.target_humidity(idx);
}

inline float humidifier_current_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.current_humidity(idx);
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

// HVAC mode to send when turning a thermostat back on: the last active mode
// this thermostat reported since boot, else the first of heat_cool/heat/cool/
// auto it supports, else its first mode that isn't "off". Empty until the
// thermostat's hvac_modes have synced.
inline std::string climate_turn_on_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  const std::string &last_active = climate_status_tracker_storage.last_active_mode(idx);
  if (!last_active.empty()) {
    return last_active;
  }
  const std::string &modes = selected_climate_hvac_modes(idx);
  for (const char *preferred : {"heat_cool", "heat", "cool", "auto"}) {
    bool supported = false;
    for_each_delimited_option(modes, [&](size_t offset, size_t len) {
      supported = modes.compare(offset, len, preferred) == 0;
      return !supported;
    });
    if (supported) {
      return preferred;
    }
  }
  std::string fallback;
  for_each_delimited_option(modes, [&](size_t offset, size_t len) {
    if (modes.compare(offset, len, "off") == 0) {
      return true;
    }
    fallback = modes.substr(offset, len);
    return false;
  });
  return fallback;
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

inline const std::string &selected_lock_state(int idx) {
  ensure_remote_status_trackers();
  return lock_status_tracker_storage.state(idx);
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

inline const std::string &sensor_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.state(idx);
}

inline const std::string &sensor_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.unit(idx);
}

inline const std::string &automation_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.state(idx);
}

inline const std::string &alarm_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return alarm_status_tracker_storage.state(idx);
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
