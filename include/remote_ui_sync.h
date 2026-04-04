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
  float *selected_climate_target_temp_low = nullptr;
  float *selected_climate_target_temp_high = nullptr;
  float *selected_climate_current_temp = nullptr;
  std::string *selected_climate_hvac_action = nullptr;
  std::string *selected_climate_preset = nullptr;
  std::string *last_climate_mode = nullptr;
  int *selected_media_volume_pct = nullptr;
  std::string *selected_media_title = nullptr;
  std::string *selected_media_artist = nullptr;
  std::string *selected_media_device_class = nullptr;
  std::string *selected_media_source = nullptr;
  std::string *selected_automation_state = nullptr;
  float *selected_weather_temperature = nullptr;
  float *selected_weather_humidity = nullptr;
  float *selected_weather_high_temp = nullptr;
  float *selected_weather_low_temp = nullptr;
  std::string *selected_weather_condition = nullptr;
  int *selected_notification_index = nullptr;
  bool *updated_ui = nullptr;
  bool refresh_requested = false;
  int refresh_idx = -1;
};

void sync_remote_ui_state(RemoteMode mode, int idx, RemoteUiSyncState &ui);

}  // namespace esphome
