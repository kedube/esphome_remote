#pragma once

#include <ctime>
#include <type_traits>

#include <esp_heap_caps.h>

#include "entity_trackers.h"
#include "oled_snapshot.h"
#include "ui_state_helpers.h"
#include "remote_ui_runtime.h"

inline LightStatusTracker light_status_tracker_storage;
inline SwitchStatusTracker switch_status_tracker_storage(REMOTE_MODE_SWITCHES);
inline FanStatusTracker fan_status_tracker_storage;
inline HumidifierStatusTracker humidifier_status_tracker_storage;
inline ClimateStatusTracker climate_status_tracker_storage;
inline WaterHeaterStatusTracker water_heater_status_tracker_storage;
inline LockStatusTracker lock_status_tracker_storage;
inline CoverStatusTracker cover_status_tracker_storage;
inline MediaStatusTracker media_status_tracker_storage;
inline SensorStatusTracker sensor_status_tracker_storage;
inline AutomationStatusTracker automation_status_tracker_storage;
inline AlarmStatusTracker alarm_status_tracker_storage;
inline NotificationFeedTracker notification_feed_tracker_storage;
inline WeatherStatusTracker weather_status_tracker_storage;
inline SunStateTracker sun_state_tracker_storage;
inline InputStatusTracker input_status_tracker_storage;
inline VacuumStatusTracker vacuum_status_tracker_storage;
inline TimerStatusTracker timer_status_tracker_storage;
inline RemoteStatusTracker remote_status_tracker_storage;
inline bool remote_status_trackers_initialized = false;

// A button was pressed since the remote woke (set by reset_idle_timer). Until
// then the remote isn't in use, so it may restart to use new favorite lists.
inline bool remote_buttons_pressed_since_boot = false;

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
      if (strcmp(entity_id, favorite_list_entry(this->favorite_list_index, i)->entity_id) == 0) {
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
  for (int i = 0; i < tracker.count(); i++) {
    if (order.rank(tracker.entity_id(i)) == rank) {
      tracker.subscribe(i);
    }
  }
}

// Calls fn on every entity tracker.
template <typename Fn>
inline void for_each_entity_tracker(Fn fn) {
  fn(light_status_tracker_storage);
  fn(switch_status_tracker_storage);
  fn(fan_status_tracker_storage);
  fn(humidifier_status_tracker_storage);
  fn(climate_status_tracker_storage);
  fn(water_heater_status_tracker_storage);
  fn(lock_status_tracker_storage);
  fn(cover_status_tracker_storage);
  fn(media_status_tracker_storage);
  fn(sensor_status_tracker_storage);
  fn(automation_status_tracker_storage);
  fn(alarm_status_tracker_storage);
  fn(weather_status_tracker_storage);
  fn(input_status_tracker_storage);
  fn(vacuum_status_tracker_storage);
  fn(timer_status_tracker_storage);
  fn(remote_status_tracker_storage);
}

// About how much memory the trackers and their subscriptions take for a set
// of favorite lists. Values Home Assistant sends (titles, option lists) come
// on top.
inline size_t favorites_memory_estimate(const FavoriteSet &favorites) {
  constexpr size_t SUBSCRIPTION_BYTES = sizeof(esphome::api::APIServer::HomeAssistantStateSubscription);
  size_t bytes = favorites.text().size() + favorites.favorite_count() * 2 * sizeof(FavoriteEntity);
  for_each_entity_tracker([&](auto &tracker) {
    using Tracker = std::remove_reference_t<decltype(tracker)>;
    // The API server keeps its subscriptions in a vector, which can hold
    // twice what it uses.
    bytes += favorites.mode_count(tracker.tracked_mode()) *
             (sizeof(typename Tracker::SlotType) + Tracker::SUBSCRIPTIONS * 2 * SUBSCRIPTION_BYTES);
  });
  return bytes;
}

// Memory the remote keeps free for everything else: values Home Assistant
// sends, Wi-Fi and the API connection.
inline constexpr size_t FAVORITES_MEMORY_RESERVE = 16 * 1024;

// Whether new lists from Home Assistant fit in memory alongside everything
// else, judged by what the lists in use take and what is free now.
inline std::string favorites_fit_in_memory(const FavoriteSet &candidate, const FavoriteSet &current) {
  size_t need = favorites_memory_estimate(candidate);
  size_t free_now = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t available = favorites_memory_estimate(current) + free_now;
  ESP_LOGI("favorites", "New lists need about %u bytes; %u in use, %u free", static_cast<unsigned>(need),
           static_cast<unsigned>(favorites_memory_estimate(current)), static_cast<unsigned>(free_now));
  if (need + FAVORITES_MEMORY_RESERVE <= available) {
    return "";
  }
  size_t room = available > FAVORITES_MEMORY_RESERVE ? available - FAVORITES_MEMORY_RESERVE : 0;
  return "too big for the remote's memory: it needs about " + std::to_string((need + 1023) / 1024) +
         " KB, and about " + std::to_string(room / 1024) + " KB is free for favorites";
}

// Sets up all trackers once. State is delivered exclusively through the
// subscriptions registered here: Home Assistant pushes the current value of
// every subscribed state/attribute right after the API handshake (and again on
// every reconnect), then streams changes, so nothing ever needs to "refresh"
// a tracker — issuing explicit fetches via get_home_assistant_state() would
// permanently grow the API server's subscription vector without ever being
// announced to Home Assistant once the handshake is done.
//
// The favorite lists Home Assistant publishes come last: they are only used
// from the next wake, so nothing on screen waits for them.
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
  for_each_entity_tracker([](auto &tracker) { tracker.allocate(); });
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
    subscribe_tracker_rank(input_status_tracker_storage, order, rank);
    subscribe_tracker_rank(vacuum_status_tracker_storage, order, rank);
    subscribe_tracker_rank(timer_status_tracker_storage, order, rank);
    subscribe_tracker_rank(remote_status_tracker_storage, order, rank);
  }
  if (mode_entity_count(REMOTE_MODE_WEATHER) > 0) {
    sun_state_tracker_storage.subscribe();
  }
  if (favorites_from_home_assistant_enabled()) {
    ha_subscribe(FAVORITES_ENTITY, FAVORITES_ATTRIBUTE, [](esphome::StringRef state) {
      favorites_received(state.c_str(), state.size(), favorites_fit_in_memory, !remote_buttons_pressed_since_boot);
    });
  }
  remote_status_trackers_initialized = true;
}

// The entity the menu position selects at boot: a favorite, the
// notification feed or an Info page.
inline const char *menu_position_entity(int menu_index, int info_index) {
  if (menu_index_is_favorite(menu_index)) {
    const FavoriteEntity *entry = favorite_list_entry(menu_index, favorite_selected_index_ref(menu_index));
    return entry != nullptr ? entry->entity_id : "";
  }
  if (menu_index_is_notifications(menu_index)) {
    return NotificationFeedTracker::entity_cstr();
  }
  return indexed_value_cstr(INFO_ITEM_ENTITIES, INFO_ITEM_COUNT, info_index);
}

// The item the remote was on before it slept or restarted, by the hash
// persist_ui_state kept of its entity. The saved menu position is a number,
// so when the favorite lists have changed it may now hold another item, or
// Notifications and Info (which follow the lists) may have moved: find the
// item in the lists in use, the same list first. Then drops the wake
// snapshot if the remote isn't on what it shows.
inline void follow_saved_selection(int &menu_index, uint32_t saved_hash, int info_index) {
  auto is_saved = [saved_hash](const char *entity) {
    return entity != nullptr && entity[0] != '\0' && ui_state_entity_hash(entity) == saved_hash;
  };
  if (saved_hash != 0 && !is_saved(menu_position_entity(menu_index, info_index))) {
    bool found = false;
    if (notifications_mode_enabled() && is_saved(NotificationFeedTracker::entity_cstr())) {
      menu_index = notifications_menu_index();
      found = true;
    }
    for (int i = 0; i < INFO_ITEM_COUNT && !found; i++) {
      if (is_saved(INFO_ITEM_ENTITIES[i])) {
        menu_index = info_menu_index();
        found = true;
      }
    }
    for (int pass = 0; pass < 2 && !found; pass++) {
      for (int list = 0; list < favorite_list_count() && !found; list++) {
        if ((pass == 0) != (list == menu_index)) {
          continue;
        }
        for (int item = 0; item < favorite_list_item_count(list) && !found; item++) {
          if (is_saved(favorite_list_entry(list, item)->entity_id)) {
            menu_index = list;
            favorite_selected_index_ref(list) = item;
            found = true;
          }
        }
      }
    }
  }
  if (esphome::oled_snapshot_valid() && !esphome::oled_snapshot_entity_is(menu_position_entity(menu_index, info_index))) {
    esphome::oled_snapshot_discard();
  }
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

// -1 until Home Assistant has sent the light's color modes, 0 for an on/off
// light, 1 for one that dims.
inline int selected_light_dimmable(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.dimmable(idx);
}

inline bool selected_light_supports_color_temp(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.supports_color_temp(idx);
}

inline float selected_light_color_temp(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.color_temp_kelvin(idx);
}

inline float selected_light_min_kelvin(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.min_color_temp_kelvin(idx);
}

inline float selected_light_max_kelvin(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.max_color_temp_kelvin(idx);
}

inline bool selected_light_supports_color(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.supports_color(idx);
}

// The LIGHT_COLOR_PRESETS entry nearest the light's colour; -1 when it
// reports none (it is off). Home Assistant reports an hs_color for a light in
// its colour-temperature mode too (a warm white is orange), but a colour
// temperature only then: that light is white.
inline int selected_light_color_index(int idx) {
  ensure_remote_status_trackers();
  if (light_status_tracker_storage.supports_color_temp(idx) &&
      !std::isnan(light_status_tracker_storage.color_temp_kelvin(idx)) &&
      !std::isnan(light_status_tracker_storage.hue(idx))) {
    return 0;
  }
  return light_color_preset_index(light_status_tracker_storage.hue(idx), light_status_tracker_storage.saturation(idx));
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

// Whether the fan has speeds, even before it reports one (see the tracker).
inline bool selected_fan_supports_speed(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.supports_speed(idx);
}

// The fan's speed increment (33.3 for a 3-speed fan); 10 until it has synced.
inline float selected_fan_percentage_step(int idx) {
  ensure_remote_status_trackers();
  float step = fan_status_tracker_storage.percentage_step(idx);
  return std::isnan(step) || step < 1.0f ? 10.0f : step;
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

// A humidity target within the humidifier's own limits, which Home Assistant
// enforces. 0-100 until those have synced.
inline float clamp_humidifier_target(int idx, float target) {
  ensure_remote_status_trackers();
  float low = humidifier_status_tracker_storage.min_humidity(idx);
  float high = humidifier_status_tracker_storage.max_humidity(idx);
  low = std::isnan(low) ? 0.0f : low;
  high = std::isnan(high) || high < low ? 100.0f : high;
  return target < low ? low : target > high ? high : target;
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

inline const std::string &climate_swing_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.swing_mode(idx);
}

inline const std::string &climate_swing_modes_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.swing_modes(idx);
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

// A setpoint within the thermostat's min_temp/max_temp, which Home Assistant
// enforces. Unclamped until those have synced.
inline float clamp_climate_temperature(int idx, float target) {
  ensure_remote_status_trackers();
  float low = climate_status_tracker_storage.min_temperature(idx);
  float high = climate_status_tracker_storage.max_temperature(idx);
  if (!std::isnan(low) && target < low) {
    return low;
  }
  if (!std::isnan(high) && target > high) {
    return high;
  }
  return target;
}

// A humidity target within the thermostat's own limits; Home Assistant's
// defaults (30-99%) until those have synced.
inline float clamp_climate_humidity(int idx, float target) {
  ensure_remote_status_trackers();
  float low = climate_status_tracker_storage.min_humidity(idx);
  float high = climate_status_tracker_storage.max_humidity(idx);
  low = std::isnan(low) ? 30.0f : low;
  high = std::isnan(high) || high < low ? 99.0f : high;
  return target < low ? low : target > high ? high : target;
}

// How many HVAC modes the thermostat offers; 0 until they have synced.
inline int climate_hvac_mode_count(int idx) {
  ensure_remote_status_trackers();
  return delimited_option_count(climate_status_tracker_storage.hvac_modes(idx));
}

// Whether the thermostat can be switched off: hvac_modes includes "off", or
// hasn't synced yet.
inline bool climate_supports_off(int idx) {
  ensure_remote_status_trackers();
  const std::string &modes = climate_status_tracker_storage.hvac_modes(idx);
  if (modes.empty()) {
    return true;
  }
  bool found = false;
  for_each_delimited_option(modes, [&](size_t offset, size_t len) {
    found = modes.compare(offset, len, "off") == 0;
    return !found;
  });
  return found;
}

inline const std::string &selected_water_heater_state(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.state(idx);
}

inline float selected_water_heater_target_temperature(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.target_temperature(idx);
}

inline float selected_water_heater_min_temperature(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.min_temperature(idx);
}

inline float selected_water_heater_max_temperature(int idx) {
  ensure_remote_status_trackers();
  return water_heater_status_tracker_storage.max_temperature(idx);
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

// water_heater.turn_on/turn_off only work on a heater with the on/off feature
// (8). Until its features sync, assume it has it.
inline bool water_heater_supports_on_off(int idx) {
  ensure_remote_status_trackers();
  int features = water_heater_status_tracker_storage.supported_features(idx);
  return features < 0 || (features & 8) != 0;
}

inline bool water_heater_has_operation(int idx, const char *operation) {
  ensure_remote_status_trackers();
  const std::string &modes = water_heater_status_tracker_storage.operation_list(idx);
  bool found = false;
  for_each_delimited_option(modes, [&](size_t offset, size_t len) {
    found = modes.compare(offset, len, operation) == 0;
    return !found;
  });
  return found;
}

// The operation that turns a heater without the on/off feature back on: its
// last one other than "off", else the first such operation it lists. Empty
// when there is none.
inline std::string water_heater_turn_on_operation(int idx) {
  ensure_remote_status_trackers();
  const std::string &last_active = water_heater_status_tracker_storage.last_active_mode(idx);
  if (!last_active.empty()) {
    return last_active;
  }
  const std::string &modes = water_heater_status_tracker_storage.operation_list(idx);
  std::string first;
  for_each_delimited_option(modes, [&](size_t offset, size_t len) {
    if (modes.compare(offset, len, "off") == 0) {
      return true;
    }
    first = modes.substr(offset, len);
    return false;
  });
  return first;
}

inline const std::string &selected_lock_state(int idx) {
  ensure_remote_status_trackers();
  return lock_status_tracker_storage.state(idx);
}

// The lock can be opened (unlatched); false until its features have synced.
inline bool selected_lock_supports_open(int idx) {
  ensure_remote_status_trackers();
  return lock_status_tracker_storage.supports_open(idx);
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

inline bool selected_cover_supports_stop(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.supports_stop(idx);
}

// Square and Circle stop the cover or valve straight away: it has the stop
// feature and Home Assistant reports it moving. Not within the few seconds
// after Plus or Minus moved it to a position, while the footer still shows
// that position and the remote shows the move it asked for rather than what
// Home Assistant reports.
inline bool selected_cover_stoppable(int idx) {
  return selected_cover_supports_stop(idx) && cover_state_moving(selected_cover_state(idx)) &&
         !esphome::local_change_hold_active(millis());
}

inline bool selected_cover_is_valve(int idx) {
  return cover_status_tracker_storage.is_valve(idx);
}

// The service that does verb ("open", "close", "stop") to this cover or valve:
// cover.open_cover, valve.stop_valve.
inline std::string cover_action_for_index(int idx, const char *verb) {
  return cover_domain_action(mode_item_entity_cstr(REMOTE_MODE_COVERS, idx), verb);
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

// -1 for a player that doesn't report mute (or is off), 0 or 1.
inline int media_muted_for_index(int idx) {
  ensure_remote_status_trackers();
  const std::string &muted = media_status_tracker_storage.muted(idx);
  if (muted.empty() || ha_state_missing(muted)) {
    return -1;
  }
  return muted == "True" || muted == "true" || muted == "on" ? 1 : 0;
}

inline const std::string &sensor_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.state(idx);
}

inline const std::string &sensor_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.unit(idx);
}

inline bool sensor_is_presence(int idx) {
  return sensor_status_tracker_storage.is_presence(idx);
}

inline bool sensor_is_event(int idx) {
  return sensor_status_tracker_storage.is_event(idx);
}

inline const std::string &sensor_event_type_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.event_type(idx);
}

inline const std::string &sensor_device_class_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.device_class(idx);
}

// Seconds since an event entity last fired, by the clock Home Assistant sets;
// -1 when that isn't known (it never has, or the clock isn't set).
inline int64_t sensor_event_seconds_ago(int idx) {
  ensure_remote_status_trackers();
  int64_t epoch = 0;
  bool date_only = false;
  time_t now = ::time(nullptr);
  if (!sensor_is_event(idx) || now < 1600000000 ||
      !parse_ha_timestamp(sensor_status_tracker_storage.state(idx), &epoch, &date_only) || date_only) {
    return -1;
  }
  int64_t ago = static_cast<int64_t>(now) - epoch;
  return ago < 0 ? 0 : ago;
}

inline const std::string &automation_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.state(idx);
}

inline const std::string &automation_last_triggered_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.last_triggered(idx);
}

// Whether an automation has run since the remote asked it to at requested_at
// (seconds since 1970 by the clock Home Assistant sets; 0 if that wasn't set):
// its last_triggered is that recent. Home Assistant writes it as
// "2026-10-02 21:30:00.123456+00:00". Without a clock, whether it changed
// from before, which only works once it has synced.
inline bool automation_ran_since(int idx, uint32_t requested_at, const std::string &before) {
  const std::string &last = automation_last_triggered_for_index(idx);
  int64_t epoch = 0;
  bool date_only = false;
  if (requested_at != 0 && parse_ha_timestamp(last, &epoch, &date_only) && !date_only) {
    return epoch + 1 >= static_cast<int64_t>(requested_at);
  }
  return !before.empty() && last != before;
}

// A script that ignores a start while it runs ("single", the default mode).
inline bool script_ignores_restart(int idx) {
  ensure_remote_status_trackers();
  const std::string &mode = automation_status_tracker_storage.mode(idx);
  return mode.empty() || mode == "single";
}

inline const std::string &alarm_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return alarm_status_tracker_storage.state(idx);
}

// -1 until Home Assistant has sent the panel's features.
inline int alarm_supported_features_for_index(int idx) {
  ensure_remote_status_trackers();
  return alarm_status_tracker_storage.supported_features(idx);
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

inline const char *weather_entity_id_for_index(int idx) {
  return mode_item_entity_cstr(REMOTE_MODE_WEATHER, idx);
}

// Whether fetch_weather_forecast should ask now: once per wake, again if Home
// Assistant never answered, and once more for twice-daily forecasts.
inline bool weather_forecast_due(int idx, uint32_t now) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.forecast_due(idx, now);
}

inline void mark_weather_forecast_sent(int idx, uint32_t now) {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.mark_forecast_sent(idx, now);
}

inline const char *weather_forecast_type_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.forecast_type(idx);
}

inline void weather_forecast_failed(int idx) {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.forecast_failed(idx);
}

// The reply fetch_weather_forecast's response_template shapes:
// {"response": {"high": 72, "low": 55, "precipitation": 0.1}}. A value the
// forecast doesn't have arrives as null and is stored as NaN.
inline void store_weather_forecast(int idx, JsonObjectConst reply) {
  ensure_remote_status_trackers();
  JsonObjectConst today = reply["response"];
  auto number = [&today](const char *key) -> float {
    JsonVariantConst value = today[key];
    return value.is<float>() ? value.as<float>() : NAN;
  };
  weather_status_tracker_storage.store_forecast(idx, number("high"), number("low"), number("precipitation"));
}

inline const std::string &weather_temperature_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.temperature_unit(idx);
}

inline const std::string &weather_wind_speed_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.wind_speed_unit(idx);
}

inline const std::string &weather_pressure_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.pressure_unit(idx);
}

inline const std::string &weather_precipitation_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.precipitation_unit(idx);
}

// 1 when sun.sun is below the horizon, 0 above it, -1 when unknown.
inline int weather_sun_below_horizon() {
  ensure_remote_status_trackers();
  return sun_state_tracker_storage.below_horizon();
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

inline const std::string &input_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return input_status_tracker_storage.state(idx);
}

inline bool input_is_select(int idx) {
  return input_status_tracker_storage.is_select(idx);
}

inline const std::string &input_options_for_index(int idx) {
  ensure_remote_status_trackers();
  return input_status_tracker_storage.options(idx);
}

inline const std::string &input_unit_for_index(int idx) {
  ensure_remote_status_trackers();
  return input_status_tracker_storage.unit(idx);
}

inline float input_min_for_index(int idx) {
  ensure_remote_status_trackers();
  return input_status_tracker_storage.min(idx);
}

inline float input_max_for_index(int idx) {
  ensure_remote_status_trackers();
  return input_status_tracker_storage.max(idx);
}

inline float input_step_for_index(int idx) {
  ensure_remote_status_trackers();
  return input_status_tracker_storage.step(idx);
}

// A number's value, NAN until it has synced (or for a select).
inline float input_value_for_index(int idx) {
  double value = 0;
  return !input_is_select(idx) && parse_ha_number(input_state_for_index(idx), &value) ? static_cast<float>(value) : NAN;
}

inline const std::string &vacuum_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return vacuum_status_tracker_storage.state(idx);
}

inline bool vacuum_is_mower(int idx) {
  return vacuum_status_tracker_storage.is_mower(idx);
}

inline const std::string &vacuum_fan_speed_for_index(int idx) {
  ensure_remote_status_trackers();
  return vacuum_status_tracker_storage.fan_speed(idx);
}

inline const std::string &vacuum_fan_speed_list_for_index(int idx) {
  ensure_remote_status_trackers();
  return vacuum_status_tracker_storage.fan_speed_list(idx);
}

inline const std::string &timer_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return timer_status_tracker_storage.state(idx);
}

// Seconds left on the timer by the clock Home Assistant sets; -1 until known.
inline int64_t timer_seconds_left_for_index(int idx) {
  ensure_remote_status_trackers();
  time_t now = ::time(nullptr);
  return timer_status_tracker_storage.seconds_left(idx, now > 1600000000 ? static_cast<int64_t>(now) : 0);
}

inline const std::string &remote_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return remote_status_tracker_storage.state(idx);
}

inline const std::string &remote_activity_for_index(int idx) {
  ensure_remote_status_trackers();
  return remote_status_tracker_storage.activity(idx);
}

inline const std::string &remote_activity_list_for_index(int idx) {
  ensure_remote_status_trackers();
  return remote_status_tracker_storage.activity_list(idx);
}

// The remote has a command set, so its keys and arrows can be sent.
inline bool remote_has_commands(int idx) {
  const char *commands = remote_status_tracker_storage.commands(idx);
  return commands != nullptr && commands[0] != '\0';
}

// What remote.send_command takes for key on this remote; empty for none.
inline std::string remote_command_for_index(int idx, int key) {
  return remote_command_for_key(remote_status_tracker_storage.commands(idx), key);
}

// The device its commands go to, for a remote that needs one; else empty.
inline std::string remote_device_for_index(int idx) {
  return remote_command_device(remote_status_tracker_storage.commands(idx));
}

// Whether Home Assistant has sent this entity's state since the remote woke.
inline bool selected_entity_synced(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS: return !selected_light_state(idx).empty();
    case REMOTE_MODE_SWITCHES: return !selected_switch_state(idx).empty();
    case REMOTE_MODE_CLIMATE: return !selected_climate_state(idx).empty();
    case REMOTE_MODE_WATER_HEATERS: return !selected_water_heater_state(idx).empty();
    case REMOTE_MODE_HUMIDIFIERS: return !selected_humidifier_state(idx).empty();
    case REMOTE_MODE_FANS: return !selected_fan_state(idx).empty();
    case REMOTE_MODE_COVERS: return !selected_cover_state(idx).empty();
    case REMOTE_MODE_LOCKS: return !selected_lock_state(idx).empty();
    case REMOTE_MODE_MEDIA: return !selected_media_state(idx).empty();
    case REMOTE_MODE_SENSORS: return !sensor_state_for_index(idx).empty();
    case REMOTE_MODE_AUTOMATION: return !automation_state_for_index(idx).empty();
    case REMOTE_MODE_ALARMS: return !alarm_state_for_index(idx).empty();
    case REMOTE_MODE_WEATHER: return !weather_state_for_index(idx).empty();
    case REMOTE_MODE_INPUTS: return !input_state_for_index(idx).empty();
    case REMOTE_MODE_VACUUMS: return !vacuum_state_for_index(idx).empty();
    case REMOTE_MODE_TIMERS: return !timer_state_for_index(idx).empty();
    case REMOTE_MODE_REMOTES: return !remote_state_for_index(idx).empty();
    default: return true;
  }
}

// A queued press (see RemoteQueuedPress) can be decided on: Home Assistant is
// connected, and the selected item's state arrived QUEUED_PRESS_SETTLE_MS ago.
// mode and idx are the selected item's.
inline bool queued_press_ready(uint32_t now, RemoteMode mode, int idx) {
  bool connected = ha_api_ready() && esphome::api::global_api_server->is_connected_with_state_subscription();
  return esphome::queued_press_settled(now, connected && selected_entity_synced(mode, idx));
}

// What the button prompts need about the selected entity of mode. Takes the
// selected index of each mode that has any, and the selected setting.
inline RemoteEntityTraits selected_entity_traits(RemoteMode mode, int alarm_idx, int cover_idx, int automation_idx,
                                                 int setting = REMOTE_SETTING_NONE) {
  RemoteEntityTraits traits;
  switch (mode) {
    case REMOTE_MODE_LOCKS:
      traits.lock_open = setting == REMOTE_SETTING_LOCK_OPEN;
      break;
    case REMOTE_MODE_ALARMS:
      traits.alarm_features = alarm_supported_features_for_index(alarm_idx);
      break;
    case REMOTE_MODE_COVERS:
      traits.cover_stoppable = selected_cover_stoppable(cover_idx);
      break;
    case REMOTE_MODE_AUTOMATION:
      traits.automation_kind = automation_kind(automation_idx);
      break;
    default:
      break;
  }
  return traits;
}
