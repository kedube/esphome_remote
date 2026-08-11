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

void format_climate_target_detail(
    char *buffer, size_t buffer_size, bool dual_target, float low, float high, float single_target,
    const char *temperature_unit) {
  if (dual_target) {
    snprintf(buffer, buffer_size, "LOW: %.0f°%s   HIGH: %.0f°%s", low, temperature_unit, high, temperature_unit);
  } else {
    snprintf(buffer, buffer_size, "TARGET: %.0f°%s", single_target, temperature_unit);
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
    // "12:59 PM" and "08/11/2026" fit these exactly; the slack is so a stray
    // out-of-range field from the time source truncates instead of surprising
    // the reader with a buffer sized to the byte.
    char time_buffer[16];
    char date_buffer[16];
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
