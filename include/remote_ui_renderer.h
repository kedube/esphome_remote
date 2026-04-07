#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "entity_helpers_requests.h"

namespace esphome {
namespace display {
class Display;
}
namespace font {
class Font;
}

struct RemoteRenderContext {
  RemoteMode mode = REMOTE_MODE_INFO;
  uint32_t now = 0;
  int contrast = 5;
  int automation_index = 0;
  int info_index = 0;
  int selected_setting_option = 0;
  int selected_weather_detail_index = 0;
  int climate_target_focus = 0;
  int selected_brightness_pct = 0;
  int selected_fan_speed_pct = 0;
  int selected_cover_position_pct = 0;
  int selected_media_volume_pct = 0;
  int battery_percentage = 0;
  uint32_t last_brightness_interaction = 0;
  uint32_t last_switch_interaction = 0;
  uint32_t last_contrast_interaction = 0;
  uint32_t last_setting_interaction = 0;
  uint32_t last_fan_speed_interaction = 0;
  uint32_t last_humidifier_interaction = 0;
  uint32_t last_humidifier_mode_interaction = 0;
  uint32_t last_lock_interaction = 0;
  uint32_t last_cover_interaction = 0;
  uint32_t last_cover_position_interaction = 0;
  uint32_t last_climate_target_focus_interaction = 0;
  uint32_t last_media_volume_interaction = 0;
  uint32_t last_media_source_interaction = 0;
  uint32_t last_media_power_interaction = 0;
  uint32_t last_automation_interaction = 0;
  uint32_t last_alarm_interaction = 0;
  uint32_t last_notification_dismiss_interaction = 0;
  uint32_t settings_button_press_started_at = 0;
  int settings_button_press_mode = -1;
  bool settings_button_long_press_fired = false;
  bool battery_monitoring_available = false;
  bool weather_is_night = false;
  float selected_humidifier_target_humidity = NAN;
  float selected_humidifier_current_humidity = NAN;
  float selected_climate_target_temp = NAN;
  float selected_water_heater_target_temp = NAN;
  float selected_climate_target_temp_low = NAN;
  float selected_climate_target_temp_high = NAN;
  float selected_climate_current_temp = NAN;
  float selected_climate_target_humidity = NAN;
  float climate_target_focus_value = NAN;
  float selected_weather_temperature = NAN;
  float selected_weather_humidity = NAN;
  float selected_weather_high_temp = NAN;
  float selected_weather_low_temp = NAN;
  float battery_voltage = NAN;
  const std::string *selected_item_name = nullptr;
  const std::string *selected_item_state = nullptr;
  const std::string *selected_humidifier_action = nullptr;
  const std::string *selected_humidifier_mode = nullptr;
  const std::string *selected_climate_hvac_action = nullptr;
  const std::string *selected_climate_fan_mode = nullptr;
  const std::string *selected_climate_hvac_mode = nullptr;
  const std::string *selected_media_title = nullptr;
  const std::string *selected_media_artist = nullptr;
  const std::string *selected_media_device_class = nullptr;
  const std::string *selected_media_source = nullptr;
  const std::string *selected_media_shuffle = nullptr;
  const std::string *selected_media_repeat = nullptr;
  const std::string *selected_media_sound_mode = nullptr;
  const std::string *selected_water_heater_mode = nullptr;
  const std::string *selected_water_heater_away = nullptr;
  const std::string *last_media_power_feedback = nullptr;
  const std::string *last_automation_feedback = nullptr;
  const std::string *last_alarm_feedback = nullptr;
  const std::string *last_switch_feedback = nullptr;
  const std::string *last_lock_feedback = nullptr;
  const std::string *last_cover_feedback = nullptr;
  const std::string *selected_weather_condition = nullptr;
  const std::string *selected_sensor_unit = nullptr;
  const std::string *selected_setting_detail = nullptr;
  std::string info_primary_text;
  std::string info_secondary_text;
  const char *temperature_unit = "F";
};

void render_remote_ui(
    display::Display *display, font::Font *tiny_font, font::Font *small_font, font::Font *medium_font,
    font::Font *symbols, font::Font *medium_symbols, font::Font *weather_symbols, const RemoteRenderContext &ctx);

}  // namespace esphome
