#include "remote_ui_logic.h"

#include <cmath>
#include <cstdio>

#include "esphome/components/network/ip_address.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/version.h"

namespace esphome {

bool remote_ui_has_dual_climate_target(
    const std::string &selected_item_state, float target_temp_low, float target_temp_high) {
  return !std::isnan(target_temp_low) &&
         !std::isnan(target_temp_high) &&
         (selected_item_state == "heat_cool" || target_temp_low != target_temp_high);
}

static inline RemoteUiActionCommand resolve_primary_action(RemoteMode mode, bool dual_climate_target) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return REMOTE_UI_ACTION_TOGGLE_LIGHT;
    case REMOTE_MODE_FANS:
      return REMOTE_UI_ACTION_TOGGLE_FAN;
    case REMOTE_MODE_HUMIDIFIERS:
      return REMOTE_UI_ACTION_CYCLE_HUMIDIFIER_MODE;
    case REMOTE_MODE_SWITCHES:
      return REMOTE_UI_ACTION_TURN_ON_SWITCH;
    case REMOTE_MODE_CLIMATE:
      return dual_climate_target ? REMOTE_UI_ACTION_CLIMATE_TEMP_HIGH_UP : REMOTE_UI_ACTION_MARK_UI;
    case REMOTE_MODE_LOCKS:
      return REMOTE_UI_ACTION_LOCK;
    case REMOTE_MODE_COVERS:
      return REMOTE_UI_ACTION_OPEN_COVER;
    case REMOTE_MODE_MEDIA:
      return REMOTE_UI_ACTION_SELECT_NEXT_ITEM;
    case REMOTE_MODE_AUTOMATION:
      return REMOTE_UI_ACTION_TRIGGER_AUTOMATION;
    case REMOTE_MODE_ALARMS:
      return REMOTE_UI_ACTION_ARM_ALARM;
    case REMOTE_MODE_NOTIFICATIONS:
      return REMOTE_UI_ACTION_DISMISS_NOTIFICATION;
    default:
      return REMOTE_UI_ACTION_MARK_UI;
  }
}

static inline RemoteUiActionCommand resolve_secondary_action(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_CLIMATE:
      return REMOTE_UI_ACTION_TOGGLE_CLIMATE_POWER;
    case REMOTE_MODE_HUMIDIFIERS:
      return REMOTE_UI_ACTION_TOGGLE_HUMIDIFIER_POWER;
    case REMOTE_MODE_MEDIA:
      return REMOTE_UI_ACTION_SELECT_NEXT_MEDIA_SOURCE;
    case REMOTE_MODE_ALARMS:
      return REMOTE_UI_ACTION_TRIGGER_ALARM;
    default:
      return REMOTE_UI_ACTION_MARK_UI;
  }
}

static inline RemoteUiActionCommand resolve_play_pause_action(RemoteMode mode, bool dual_climate_target) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return REMOTE_UI_ACTION_TOGGLE_LIGHT;
    case REMOTE_MODE_FANS:
      return REMOTE_UI_ACTION_TOGGLE_FAN;
    case REMOTE_MODE_SWITCHES:
      return REMOTE_UI_ACTION_TURN_OFF_SWITCH;
    case REMOTE_MODE_CLIMATE:
      return dual_climate_target ? REMOTE_UI_ACTION_CLIMATE_TEMP_HIGH_DOWN : REMOTE_UI_ACTION_MARK_UI;
    case REMOTE_MODE_LOCKS:
      return REMOTE_UI_ACTION_UNLOCK;
    case REMOTE_MODE_COVERS:
      return REMOTE_UI_ACTION_CLOSE_COVER;
    case REMOTE_MODE_MEDIA:
      return REMOTE_UI_ACTION_MEDIA_PLAY_PAUSE;
    case REMOTE_MODE_AUTOMATION:
      return REMOTE_UI_ACTION_TRIGGER_AUTOMATION;
    case REMOTE_MODE_ALARMS:
      return REMOTE_UI_ACTION_DISARM_ALARM;
    case REMOTE_MODE_NOTIFICATIONS:
      return REMOTE_UI_ACTION_DISMISS_NOTIFICATION;
    default:
      return REMOTE_UI_ACTION_MARK_UI;
  }
}

static inline RemoteUiActionCommand resolve_increase_action(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return REMOTE_UI_ACTION_INCREASE_BRIGHTNESS;
    case REMOTE_MODE_FANS:
      return REMOTE_UI_ACTION_INCREASE_FAN_SPEED;
    case REMOTE_MODE_HUMIDIFIERS:
      return REMOTE_UI_ACTION_HUMIDIFIER_TARGET_UP;
    case REMOTE_MODE_COVERS:
      return REMOTE_UI_ACTION_INCREASE_COVER_POSITION;
    case REMOTE_MODE_CLIMATE:
      return REMOTE_UI_ACTION_CLIMATE_TEMP_UP;
    case REMOTE_MODE_MEDIA:
      return REMOTE_UI_ACTION_MEDIA_VOLUME_UP;
    case REMOTE_MODE_ALARMS:
      return REMOTE_UI_ACTION_SHIFT_ALARM_MODE_FORWARD;
    default:
      return REMOTE_UI_ACTION_MARK_UI;
  }
}

static inline RemoteUiActionCommand resolve_decrease_action(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return REMOTE_UI_ACTION_DECREASE_BRIGHTNESS;
    case REMOTE_MODE_FANS:
      return REMOTE_UI_ACTION_DECREASE_FAN_SPEED;
    case REMOTE_MODE_HUMIDIFIERS:
      return REMOTE_UI_ACTION_HUMIDIFIER_TARGET_DOWN;
    case REMOTE_MODE_COVERS:
      return REMOTE_UI_ACTION_DECREASE_COVER_POSITION;
    case REMOTE_MODE_CLIMATE:
      return REMOTE_UI_ACTION_CLIMATE_TEMP_DOWN;
    case REMOTE_MODE_MEDIA:
      return REMOTE_UI_ACTION_MEDIA_VOLUME_DOWN;
    case REMOTE_MODE_ALARMS:
      return REMOTE_UI_ACTION_SHIFT_ALARM_MODE_BACKWARD;
    default:
      return REMOTE_UI_ACTION_MARK_UI;
  }
}

RemoteUiActionCommand resolve_remote_mode_action_command(RemoteMode mode, int action, bool dual_climate_target) {
  const int MODE_ACTION_PRIMARY = 0;
  const int MODE_ACTION_SECONDARY = 1;
  const int MODE_ACTION_PLAY_PAUSE = 2;
  const int MODE_ACTION_INCREASE = 3;
  const int MODE_ACTION_DECREASE = 4;

  switch (action) {
    case MODE_ACTION_PRIMARY:
      return resolve_primary_action(mode, dual_climate_target);
    case MODE_ACTION_SECONDARY:
      return resolve_secondary_action(mode);
    case MODE_ACTION_PLAY_PAUSE:
      return resolve_play_pause_action(mode, dual_climate_target);
    case MODE_ACTION_INCREASE:
      return resolve_increase_action(mode);
    case MODE_ACTION_DECREASE:
      return resolve_decrease_action(mode);
    default:
      return REMOTE_UI_ACTION_MARK_UI;
  }
}

RemoteUiActionGroup group_remote_mode_action_command(RemoteUiActionCommand command) {
  switch (command) {
    case REMOTE_UI_ACTION_TOGGLE_LIGHT:
    case REMOTE_UI_ACTION_TOGGLE_FAN:
    case REMOTE_UI_ACTION_TOGGLE_CLIMATE_POWER:
    case REMOTE_UI_ACTION_TOGGLE_HUMIDIFIER_POWER:
    case REMOTE_UI_ACTION_CYCLE_HUMIDIFIER_MODE:
    case REMOTE_UI_ACTION_TURN_ON_SWITCH:
    case REMOTE_UI_ACTION_TURN_OFF_SWITCH:
    case REMOTE_UI_ACTION_MEDIA_PLAY_PAUSE:
      return REMOTE_UI_ACTION_GROUP_TOGGLE;

    case REMOTE_UI_ACTION_SELECT_NEXT_ITEM:
    case REMOTE_UI_ACTION_TRIGGER_AUTOMATION:
    case REMOTE_UI_ACTION_DISMISS_NOTIFICATION:
    case REMOTE_UI_ACTION_SELECT_NEXT_MEDIA_SOURCE:
      return REMOTE_UI_ACTION_GROUP_SELECTION;

    case REMOTE_UI_ACTION_CLIMATE_TEMP_HIGH_UP:
    case REMOTE_UI_ACTION_CLIMATE_TEMP_HIGH_DOWN:
    case REMOTE_UI_ACTION_INCREASE_BRIGHTNESS:
    case REMOTE_UI_ACTION_DECREASE_BRIGHTNESS:
    case REMOTE_UI_ACTION_INCREASE_FAN_SPEED:
    case REMOTE_UI_ACTION_DECREASE_FAN_SPEED:
    case REMOTE_UI_ACTION_HUMIDIFIER_TARGET_UP:
    case REMOTE_UI_ACTION_HUMIDIFIER_TARGET_DOWN:
    case REMOTE_UI_ACTION_INCREASE_COVER_POSITION:
    case REMOTE_UI_ACTION_DECREASE_COVER_POSITION:
    case REMOTE_UI_ACTION_CLIMATE_TEMP_UP:
    case REMOTE_UI_ACTION_CLIMATE_TEMP_DOWN:
    case REMOTE_UI_ACTION_MEDIA_VOLUME_UP:
    case REMOTE_UI_ACTION_MEDIA_VOLUME_DOWN:
    case REMOTE_UI_ACTION_SHIFT_ALARM_MODE_FORWARD:
    case REMOTE_UI_ACTION_SHIFT_ALARM_MODE_BACKWARD:
      return REMOTE_UI_ACTION_GROUP_ADJUST;

    case REMOTE_UI_ACTION_LOCK:
    case REMOTE_UI_ACTION_UNLOCK:
    case REMOTE_UI_ACTION_OPEN_COVER:
    case REMOTE_UI_ACTION_CLOSE_COVER:
    case REMOTE_UI_ACTION_ARM_ALARM:
    case REMOTE_UI_ACTION_DISARM_ALARM:
    case REMOTE_UI_ACTION_TRIGGER_ALARM:
      return REMOTE_UI_ACTION_GROUP_SECURITY;

    case REMOTE_UI_ACTION_NONE:
    case REMOTE_UI_ACTION_MARK_UI:
    default:
      return REMOTE_UI_ACTION_GROUP_NONE;
  }
}

void populate_remote_info_text(
    int info_index, const ESPTime &time_now, const char *version, const char *device_name,
    const char *friendly_name, bool battery_monitoring_available, int battery_percentage, float battery_voltage,
    std::string &primary_text, std::string &secondary_text) {
  primary_text.clear();
  secondary_text.clear();

  if (info_index == 0) {
    if (!time_now.is_valid()) {
      primary_text = "--:--";
      secondary_text = "--/--/----";
      return;
    }

    int hour = time_now.hour % 12;
    if (hour == 0) {
      hour = 12;
    }
    char time_buffer[9];
    char date_buffer[11];
    snprintf(time_buffer, sizeof(time_buffer), "%d:%02d %s", hour, time_now.minute, time_now.hour >= 12 ? "PM" : "AM");
    snprintf(date_buffer, sizeof(date_buffer), "%02d/%02d/%04d", time_now.month, time_now.day_of_month, time_now.year);
    primary_text = time_buffer;
    secondary_text = date_buffer;
    return;
  }

  if (info_index == 1) {
    if (wifi::global_wifi_component != nullptr) {
      char ssid_buffer[wifi::SSID_BUFFER_SIZE];
      primary_text = wifi::global_wifi_component->wifi_ssid_to(ssid_buffer);
    } else {
      primary_text.clear();
    }
    if (primary_text.empty()) {
      primary_text = "DISCONNECTED";
    }
    if (wifi::global_wifi_component != nullptr) {
      char rssi_buffer[20];
      snprintf(rssi_buffer, sizeof(rssi_buffer), "%d dBm", wifi::global_wifi_component->wifi_rssi());
      secondary_text = rssi_buffer;
    }
    if (secondary_text.empty()) {
      secondary_text = "NO SIGNAL";
    }
    return;
  }

  if (info_index == 2) {
    if (wifi::global_wifi_component != nullptr) {
      auto ip_addresses = wifi::global_wifi_component->wifi_sta_ip_addresses();
      for (const auto &ip : ip_addresses) {
        if (ip.is_set()) {
          char ip_buffer[network::IP_ADDRESS_BUFFER_SIZE];
          primary_text = ip.str_to(ip_buffer);
          break;
        }
      }
    }
    if (primary_text.empty() || primary_text == "0.0.0.0") {
      primary_text = "NO IP ADDRESS";
    }
    return;
  }

  if (info_index == 3) {
    primary_text = device_name != nullptr ? device_name : "";
    secondary_text = friendly_name != nullptr ? friendly_name : "";
    return;
  }

  if (info_index == 4) {
    if (battery_monitoring_available) {
      char battery_pct_buffer[12];
      char battery_voltage_buffer[16];
      snprintf(battery_pct_buffer, sizeof(battery_pct_buffer), "%d%%", battery_percentage);
      snprintf(battery_voltage_buffer, sizeof(battery_voltage_buffer), "%.02fV", battery_voltage);
      primary_text = battery_pct_buffer;
      secondary_text = battery_voltage_buffer;
    } else {
      primary_text = "N/A";
      secondary_text = "BATTERY: N/A";
    }
    return;
  }

  primary_text = version != nullptr ? version : "";
  secondary_text = ESPHOME_VERSION;
}

}  // namespace esphome
