#pragma once

#include "entity_helpers_common.h"

struct PersistedUIStateData {
  int contrast = 0;
  int current_mode = REMOTE_MODE_LIGHTS;
  int selected_light_index = 0;
  int selected_switch_index = 0;
  int selected_climate_index = 0;
  int selected_lock_index = 0;
  int selected_cover_index = 0;
  int selected_media_index = 0;
  int selected_automation_index = 0;
  int selected_weather_index = 0;
  int selected_fan_index = 0;
  int selected_humidifier_index = 0;
  int selected_sensor_index = 0;
  int selected_alarm_index = 0;
  int selected_notification_index = 0;
  int selected_info_index = 0;
};

static constexpr uint64_t UI_STATE_FORMAT_V2 = 1ULL << 63;
static constexpr uint64_t UI_STATE_FORMAT_V3 = (1ULL << 63) | (1ULL << 62);
static constexpr uint64_t UI_STATE_FORMAT_V4 = (1ULL << 63) | (1ULL << 62) | (1ULL << 61);
static constexpr uint64_t UI_STATE_FORMAT_V5 = (1ULL << 63) | (1ULL << 62) | (1ULL << 61) | (1ULL << 60);
static constexpr uint32_t UI_STATE_AUX_FORMAT_V6 = 1UL << 31;

static inline int remap_v2_mode(int old_mode) {
  switch (old_mode) {
    case 0:
      return REMOTE_MODE_LIGHTS;
    case 1:
      return REMOTE_MODE_SWITCHES;
    case 2:
      return REMOTE_MODE_CLIMATE;
    case 3:
      return REMOTE_MODE_LOCKS;
    case 4:
      return REMOTE_MODE_COVERS;
    case 5:
      return REMOTE_MODE_MEDIA;
    case 6:
      return REMOTE_MODE_AUTOMATION;
    case 7:
      return REMOTE_MODE_WEATHER;
    case 8:
      return REMOTE_MODE_INFO;
    case 9:
      return REMOTE_MODE_FANS;
    case 10:
      return REMOTE_MODE_HUMIDIFIERS;
    default:
      return REMOTE_MODE_INFO;
  }
}

static inline int remap_v3_mode(int old_mode) {
  switch (old_mode) {
    case 0:
      return REMOTE_MODE_LIGHTS;
    case 1:
      return REMOTE_MODE_SWITCHES;
    case 2:
      return REMOTE_MODE_CLIMATE;
    case 3:
      return REMOTE_MODE_HUMIDIFIERS;
    case 4:
      return REMOTE_MODE_FANS;
    case 5:
      return REMOTE_MODE_COVERS;
    case 6:
      return REMOTE_MODE_LOCKS;
    case 7:
      return REMOTE_MODE_MEDIA;
    case 8:
      return REMOTE_MODE_AUTOMATION;
    case 9:
      return REMOTE_MODE_WEATHER;
    case 10:
      return REMOTE_MODE_INFO;
    default:
      return REMOTE_MODE_INFO;
  }
}

static inline int remap_v4_mode(int old_mode) {
  switch (old_mode) {
    case 0:
      return REMOTE_MODE_LIGHTS;
    case 1:
      return REMOTE_MODE_SWITCHES;
    case 2:
      return REMOTE_MODE_CLIMATE;
    case 3:
      return REMOTE_MODE_HUMIDIFIERS;
    case 4:
      return REMOTE_MODE_FANS;
    case 5:
      return REMOTE_MODE_COVERS;
    case 6:
      return REMOTE_MODE_LOCKS;
    case 7:
      return REMOTE_MODE_MEDIA;
    case 8:
      return REMOTE_MODE_AUTOMATION;
    case 9:
      return REMOTE_MODE_NOTIFICATIONS;
    case 10:
      return REMOTE_MODE_WEATHER;
    case 11:
      return REMOTE_MODE_INFO;
    default:
      return REMOTE_MODE_INFO;
  }
}

static inline void apply_legacy_shared_index(PersistedUIStateData &data, int shared_index) {
  data.selected_sensor_index = shared_index;
  data.selected_alarm_index = shared_index;
  data.selected_notification_index = shared_index;
  data.selected_info_index = shared_index;
}

static inline PersistedUIStateData unpack_v6_ui_state(uint64_t state, uint32_t aux_state) {
  PersistedUIStateData data;
  data.contrast = state & 0x0F;
  data.current_mode = (state >> 4) & 0x0F;
  data.selected_light_index = (state >> 8) & 0x3F;
  data.selected_switch_index = (state >> 14) & 0x3F;
  data.selected_climate_index = (state >> 20) & 0x3F;
  data.selected_lock_index = (state >> 26) & 0x3F;
  data.selected_cover_index = (state >> 32) & 0x3F;
  data.selected_media_index = (state >> 38) & 0x3F;
  data.selected_automation_index = (state >> 44) & 0x3F;
  data.selected_weather_index = (state >> 50) & 0x3F;
  data.selected_fan_index = (state >> 56) & 0x3F;
  data.selected_humidifier_index = ((aux_state & 0x0F) << 2) | ((state >> 62) & 0x03);
  data.selected_sensor_index = (aux_state >> 4) & 0x3F;
  data.selected_alarm_index = (aux_state >> 10) & 0x3F;
  data.selected_notification_index = (aux_state >> 16) & 0x3F;
  data.selected_info_index = (aux_state >> 22) & 0x1F;
  return data;
}

static inline PersistedUIStateData unpack_legacy_ui_state(uint64_t state, uint32_t aux_state) {
  PersistedUIStateData data;
  data.contrast = state & 0x0F;
  int shared_index = 0;

  if ((state & UI_STATE_FORMAT_V5) == UI_STATE_FORMAT_V5) {
    data.current_mode = (state >> 4) & 0x0F;
    data.selected_light_index = (state >> 8) & 0x3F;
    data.selected_switch_index = (state >> 14) & 0x3F;
    data.selected_climate_index = (state >> 20) & 0x3F;
    data.selected_lock_index = (state >> 26) & 0x3F;
    data.selected_cover_index = (state >> 32) & 0x3F;
    data.selected_media_index = (state >> 38) & 0x3F;
    data.selected_automation_index = (state >> 44) & 0x3F;
    data.selected_weather_index = (state >> 50) & 0x3F;
    shared_index = (state >> 56) & 0x1F;
    data.selected_fan_index = aux_state & 0x3F;
    data.selected_humidifier_index = (aux_state >> 6) & 0x3F;
  } else if ((state & UI_STATE_FORMAT_V2) != 0) {
    if ((state & UI_STATE_FORMAT_V4) == UI_STATE_FORMAT_V4) {
      data.current_mode = remap_v4_mode((state >> 4) & 0x0F);
      data.selected_light_index = (state >> 8) & 0x3F;
      data.selected_switch_index = (state >> 14) & 0x3F;
      data.selected_climate_index = (state >> 20) & 0x3F;
      data.selected_lock_index = (state >> 26) & 0x3F;
      data.selected_cover_index = (state >> 32) & 0x3F;
      data.selected_media_index = (state >> 38) & 0x3F;
      data.selected_automation_index = (state >> 44) & 0x3F;
      data.selected_weather_index = (state >> 50) & 0x3F;
      shared_index = (state >> 56) & 0x1F;
    } else if ((state & UI_STATE_FORMAT_V3) == UI_STATE_FORMAT_V3) {
      data.current_mode = remap_v3_mode((state >> 4) & 0x0F);
      data.selected_light_index = (state >> 8) & 0x3F;
      data.selected_switch_index = (state >> 14) & 0x3F;
      data.selected_climate_index = (state >> 20) & 0x3F;
      data.selected_lock_index = (state >> 26) & 0x3F;
      data.selected_cover_index = (state >> 32) & 0x3F;
      data.selected_media_index = (state >> 38) & 0x3F;
      data.selected_automation_index = (state >> 44) & 0x3F;
      data.selected_weather_index = (state >> 50) & 0x3F;
      shared_index = (state >> 56) & 0x3F;
    } else {
      data.current_mode = remap_v2_mode((state >> 4) & 0x0F);
      data.selected_light_index = (state >> 8) & 0x3F;
      data.selected_switch_index = (state >> 14) & 0x3F;
      data.selected_climate_index = (state >> 20) & 0x3F;
      data.selected_lock_index = (state >> 26) & 0x3F;
      data.selected_cover_index = (state >> 32) & 0x3F;
      data.selected_media_index = (state >> 38) & 0x3F;
      data.selected_automation_index = (state >> 44) & 0x3F;
      data.selected_weather_index = (state >> 50) & 0x3F;
      shared_index = (state >> 56) & 0x3F;
    }
  } else {
    data.current_mode = state == 0 ? REMOTE_MODE_LIGHTS : REMOTE_MODE_INFO;
    data.selected_light_index = (state >> 7) & 0x3F;
    data.selected_switch_index = (state >> 13) & 0x3F;
    data.selected_climate_index = (state >> 19) & 0x3F;
    data.selected_lock_index = (state >> 25) & 0x3F;
    data.selected_cover_index = (state >> 31) & 0x3F;
    data.selected_media_index = (state >> 37) & 0x3F;
    data.selected_automation_index = (state >> 43) & 0x3F;
    data.selected_weather_index = (state >> 49) & 0x3F;
    shared_index = (state >> 55) & 0x3F;
  }

  apply_legacy_shared_index(data, shared_index);
  return data;
}

static inline PersistedUIStateData unpack_persisted_ui_state(uint64_t state, uint32_t aux_state) {
  if ((aux_state & UI_STATE_AUX_FORMAT_V6) != 0) {
    return unpack_v6_ui_state(state, aux_state);
  }
  return unpack_legacy_ui_state(state, aux_state);
}

static inline uint64_t pack_persisted_ui_state(const PersistedUIStateData &data) {
  return uint64_t(data.contrast & 0x0F) |
         (uint64_t(data.current_mode & 0x0F) << 4) |
         (uint64_t(data.selected_light_index & 0x3F) << 8) |
         (uint64_t(data.selected_switch_index & 0x3F) << 14) |
         (uint64_t(data.selected_climate_index & 0x3F) << 20) |
         (uint64_t(data.selected_lock_index & 0x3F) << 26) |
         (uint64_t(data.selected_cover_index & 0x3F) << 32) |
         (uint64_t(data.selected_media_index & 0x3F) << 38) |
         (uint64_t(data.selected_automation_index & 0x3F) << 44) |
         (uint64_t(data.selected_weather_index & 0x3F) << 50) |
         (uint64_t(data.selected_fan_index & 0x3F) << 56) |
         (uint64_t(data.selected_humidifier_index & 0x03) << 62);
}

static inline uint32_t pack_persisted_ui_state_aux(const PersistedUIStateData &data) {
  return UI_STATE_AUX_FORMAT_V6 |
         (uint32_t((data.selected_humidifier_index >> 2) & 0x0F)) |
         (uint32_t(data.selected_sensor_index & 0x3F) << 4) |
         (uint32_t(data.selected_alarm_index & 0x3F) << 10) |
         (uint32_t(data.selected_notification_index & 0x3F) << 16) |
         (uint32_t(data.selected_info_index & 0x1F) << 22);
}
