#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "remote_ui_types.h"

namespace esphome {
namespace display {
class Display;
}
namespace font {
class Font;
}

// Fonts the renderer draws with. Filled in once at boot (see remote_control.yaml).
struct RemoteUiFonts {
  font::Font *tiny = nullptr;   // Arial Bold 9: header, chips, footer text
  font::Font *small = nullptr;  // Arial Bold 10: secondary lines, notification text
  font::Font *title = nullptr;  // Roboto Condensed Bold 14: entity names and state words
  font::Font *large = nullptr;  // Roboto Condensed Bold 28 (digits, ON/OFF): hero values
  font::Font *hero = nullptr;   // Material Symbols 24: hero badges and weather
};

inline RemoteUiFonts remote_ui_fonts;

// Everything one frame needs. Plain values and borrowed pointers only, so the
// renderer has no dependency on ESPHome globals, trackers or the API.
struct RemoteRenderContext {
  RemoteMode mode = REMOTE_MODE_INFO;
  const char *list_title = nullptr;  // header chip: favorite list, NOTIFICATIONS or INFO
  int item_index = 0;                // position within the current list
  int item_count = 0;
  uint32_t now = 0;

  // Header extras.
  bool clock_valid = false;
  int clock_hour = 0;  // 0-23
  int clock_minute = 0;
  bool battery_monitoring_available = false;
  int battery_percentage = 0;
  float battery_voltage = NAN;

  // Display contrast feedback (dimmer button).
  int contrast = 5;
  uint32_t last_contrast_interaction = 0;

  // Selected setting (cycled with the settings button, adjusted with +/-). The
  // detail holds the value this remote just sent and is cleared a few seconds
  // later, after which the renderer falls back to the entity's reported value.
  int selected_setting_option = REMOTE_SETTING_NONE;
  const std::string *selected_setting_detail = nullptr;

  // A short status message ("ALREADY ON") shown in place of the footer.
  const std::string *toast_text = nullptr;
  uint32_t toast_at = 0;

  // A protected action is being held: label and 0-100 progress, or -1.
  const char *hold_label = nullptr;
  int hold_progress = -1;

  const std::string *selected_item_name = nullptr;
  const std::string *selected_item_state = nullptr;

  // Lights.
  int selected_brightness_pct = 0;
  const std::string *light_effect = nullptr;

  // Switches.
  const std::string *last_switch_feedback = nullptr;
  uint32_t last_switch_interaction = 0;

  // Climate.
  float selected_climate_current_temp = NAN;
  float selected_climate_target_temp = NAN;
  float selected_climate_target_temp_low = NAN;
  float selected_climate_target_temp_high = NAN;
  float selected_climate_target_humidity = NAN;
  const std::string *selected_climate_hvac_action = nullptr;
  const std::string *selected_climate_fan_mode = nullptr;
  const std::string *selected_climate_preset = nullptr;
  int climate_target_focus = 0;  // 0 none, 1 low/single, 2 high
  float climate_target_focus_value = NAN;
  uint32_t last_climate_target_focus_interaction = 0;

  // Water heaters.
  float selected_water_heater_target_temp = NAN;
  const std::string *selected_water_heater_mode = nullptr;
  const std::string *selected_water_heater_away = nullptr;

  // Humidifiers.
  float selected_humidifier_target_humidity = NAN;
  float selected_humidifier_current_humidity = NAN;
  const std::string *selected_humidifier_action = nullptr;
  const std::string *selected_humidifier_mode = nullptr;

  // Fans.
  int selected_fan_speed_pct = 0;
  int fan_oscillating = -1;  // -1 unknown, 0 off, 1 on
  const std::string *fan_direction = nullptr;
  const std::string *fan_preset = nullptr;

  // Covers.
  int selected_cover_position_pct = 0;
  bool cover_has_position = false;
  int cover_tilt_pct = -1;
  const std::string *last_cover_feedback = nullptr;
  uint32_t last_cover_interaction = 0;

  // Locks.
  const std::string *last_lock_feedback = nullptr;
  uint32_t last_lock_interaction = 0;

  // Media players.
  int selected_media_volume_pct = 0;
  const std::string *selected_media_title = nullptr;
  const std::string *selected_media_artist = nullptr;
  const std::string *selected_media_device_class = nullptr;
  const std::string *selected_media_source = nullptr;
  const std::string *selected_media_shuffle = nullptr;
  const std::string *selected_media_repeat = nullptr;
  const std::string *selected_media_sound_mode = nullptr;
  const std::string *last_media_power_feedback = nullptr;
  uint32_t last_media_power_interaction = 0;

  // Sensors.
  const std::string *selected_sensor_unit = nullptr;

  // Automations, scripts and scenes.
  AutomationKind automation_kind = AUTOMATION_KIND_SCRIPT;
  const std::string *last_automation_feedback = nullptr;
  uint32_t last_automation_interaction = 0;

  // Alarm panels.
  int selected_alarm_arm_mode = ALARM_ARM_MODE_AWAY;
  const std::string *last_alarm_feedback = nullptr;
  uint32_t last_alarm_interaction = 0;

  // Notifications (the message is the selected item state).
  uint32_t last_notification_dismiss_interaction = 0;

  // Weather.
  const std::string *selected_weather_condition = nullptr;
  bool weather_is_night = false;
  float selected_weather_temperature = NAN;
  float selected_weather_humidity = NAN;
  float selected_weather_high_temp = NAN;
  float selected_weather_low_temp = NAN;
  float selected_weather_wind_speed = NAN;
  float selected_weather_wind_bearing = NAN;
  float selected_weather_wind_gust_speed = NAN;
  float selected_weather_pressure = NAN;
  float selected_weather_precipitation = NAN;
  float selected_weather_cloud_coverage = NAN;
  float selected_weather_uv_index = NAN;
  float selected_weather_dew_point = NAN;
  float selected_weather_apparent_temperature = NAN;

  // Info screens.
  int info_index = 0;
  std::string info_primary_text;
  std::string info_secondary_text;
  int wifi_rssi = 0;  // dBm, 0 when unknown
  int clock_weekday = 0;  // 1 = Sunday, 0 unknown
  int clock_day = 0;
  int clock_month = 0;

  // Units.
  const char *temperature_unit = "F";
  const char *speed_unit = "MPH";
  const char *pressure_unit = "hPa";
  const char *precipitation_unit = "in";
};

void render_remote_ui(display::Display *it, const RemoteUiFonts &fonts, const RemoteRenderContext &ctx);

enum RemoteSystemScreen {
  REMOTE_SCREEN_CONNECTING_WIFI,
  REMOTE_SCREEN_CONNECTING_API,
  REMOTE_SCREEN_WIFI_LOST,
  REMOTE_SCREEN_API_LOST,
  REMOTE_SCREEN_LOW_BATTERY,
  REMOTE_SCREEN_POWERING_OFF,
  REMOTE_SCREEN_HOLD_TO_REBOOT,
  REMOTE_SCREEN_REBOOTING,
};

struct RemoteSystemScreenInfo {
  const char *version = "";
  float battery_voltage = NAN;
  int progress = -1;  // 0-100 for the hold-to-reboot bar
};

// Full-screen status pages shown outside the normal UI (boot, connection loss,
// power off). The caller is responsible for display().
void render_system_screen(
    display::Display *it, const RemoteUiFonts &fonts, RemoteSystemScreen screen, const RemoteSystemScreenInfo &info);

}  // namespace esphome
