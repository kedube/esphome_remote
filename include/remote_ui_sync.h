#pragma once

#include <cmath>
#include <string>

#include "entity_helpers_requests.h"

namespace esphome {

struct RemoteUiSyncState {
  std::string *selected_item_name = nullptr;
  std::string *selected_item_entity = nullptr;
  std::string *selected_item_state = nullptr;
  int *selected_brightness_pct = nullptr;
  int *selected_fan_speed_pct = nullptr;
  float *selected_humidifier_target_humidity = nullptr;
  float *selected_humidifier_current_humidity = nullptr;
  std::string *selected_humidifier_action = nullptr;
  std::string *selected_humidifier_mode = nullptr;
  int *selected_cover_position_pct = nullptr;
  float *selected_climate_target_temp = nullptr;
  float *selected_water_heater_target_temp = nullptr;
  float *selected_climate_target_temp_low = nullptr;
  float *selected_climate_target_temp_high = nullptr;
  float *selected_climate_current_temp = nullptr;
  float *selected_climate_target_humidity = nullptr;
  std::string *selected_climate_hvac_action = nullptr;
  std::string *selected_climate_fan_mode = nullptr;
  std::string *selected_climate_hvac_mode = nullptr;
  std::string *selected_climate_preset = nullptr;
  std::string *last_climate_mode = nullptr;
  std::string *selected_water_heater_mode = nullptr;
  std::string *selected_water_heater_away = nullptr;
  int *selected_media_volume_pct = nullptr;
  std::string *selected_media_title = nullptr;
  std::string *selected_media_artist = nullptr;
  std::string *selected_media_device_class = nullptr;
  std::string *selected_media_source = nullptr;
  std::string *selected_media_shuffle = nullptr;
  std::string *selected_media_repeat = nullptr;
  std::string *selected_media_sound_mode = nullptr;
  std::string *selected_automation_state = nullptr;
  std::string *selected_sensor_unit = nullptr;
  float *selected_weather_temperature = nullptr;
  float *selected_weather_humidity = nullptr;
  float *selected_weather_high_temp = nullptr;
  float *selected_weather_low_temp = nullptr;
  float *selected_weather_wind_speed = nullptr;
  float *selected_weather_wind_bearing = nullptr;
  float *selected_weather_wind_gust_speed = nullptr;
  float *selected_weather_pressure = nullptr;
  float *selected_weather_cloud_coverage = nullptr;
  float *selected_weather_uv_index = nullptr;
  float *selected_weather_dew_point = nullptr;
  float *selected_weather_apparent_temperature = nullptr;
  float *selected_weather_precipitation = nullptr;
  std::string *selected_weather_condition = nullptr;
  int *selected_notification_index = nullptr;
  bool *updated_ui = nullptr;
  bool refresh_requested = false;
  int refresh_idx = -1;
};

void sync_remote_ui_state(RemoteMode mode, int idx, RemoteUiSyncState &ui);

}  // namespace esphome
