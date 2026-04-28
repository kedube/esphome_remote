#pragma once

#include "entity_helpers_common.h"

struct PersistedUIStateData {
  int contrast = 0;
  int current_menu_index = 0;
  int current_mode = REMOTE_MODE_INFO;
  int current_favorite_index = 0;
  int selected_notification_index = 0;
  int selected_info_index = 0;
  int selected_setting_option = REMOTE_SETTING_NONE;
  int selected_weather_detail_index = 0;
  int selected_alarm_arm_mode = ALARM_ARM_MODE_AWAY;
};

static constexpr uint32_t UI_STATE_AUX_FORMAT_MASK = 3UL << 30;
static constexpr uint32_t UI_STATE_AUX_FORMAT_V7 = 1UL << 31;
static constexpr uint32_t UI_STATE_AUX_FORMAT_V8 = 3UL << 30;

static inline PersistedUIStateData default_persisted_ui_state() {
  PersistedUIStateData data;
  data.contrast = 5;
  data.current_menu_index = 0;
  data.current_mode = REMOTE_MODE_INFO;
  return data;
}

static inline PersistedUIStateData unpack_persisted_ui_state(uint64_t state, uint32_t aux_state) {
  uint32_t format = aux_state & UI_STATE_AUX_FORMAT_MASK;
  if (format == UI_STATE_AUX_FORMAT_V8) {
    PersistedUIStateData data;
    data.contrast = state & 0x0F;
    data.current_menu_index = (state >> 4) & 0x1F;
    data.current_favorite_index = (state >> 9) & 0x3F;
    data.selected_notification_index = (state >> 15) & 0x3F;
    data.selected_info_index = (state >> 21) & 0x3F;
    data.current_mode = (state >> 27) & 0x0F;
    data.selected_setting_option = (state >> 31) & 0x3F;
    data.selected_weather_detail_index = (state >> 37) & 0x0F;
    data.selected_alarm_arm_mode = (state >> 41) & 0x07;
    return data;
  }

  if ((aux_state & UI_STATE_AUX_FORMAT_V7) != 0) {
    PersistedUIStateData data;
    data.contrast = state & 0x0F;
    data.current_menu_index = (state >> 4) & 0x0F;
    if (data.current_menu_index >= 0 && data.current_menu_index <= 8) {
      data.current_favorite_index = (state >> (8 + data.current_menu_index * 6)) & 0x3F;
    }
    data.selected_notification_index = aux_state & 0x3F;
    data.selected_info_index = (aux_state >> 6) & 0x3F;
    return data;
  }

  return default_persisted_ui_state();
}

static inline uint64_t pack_persisted_ui_state(const PersistedUIStateData &data) {
  return uint64_t(data.contrast & 0x0F) |
         (uint64_t(data.current_menu_index & 0x1F) << 4) |
         (uint64_t(data.current_favorite_index & 0x3F) << 9) |
         (uint64_t(data.selected_notification_index & 0x3F) << 15) |
         (uint64_t(data.selected_info_index & 0x3F) << 21) |
         (uint64_t(data.current_mode & 0x0F) << 27) |
         (uint64_t(data.selected_setting_option & 0x3F) << 31) |
         (uint64_t(data.selected_weather_detail_index & 0x0F) << 37) |
         (uint64_t(data.selected_alarm_arm_mode & 0x07) << 41);
}

static inline uint32_t pack_persisted_ui_state_aux(const PersistedUIStateData &data) {
  (void) data;
  return UI_STATE_AUX_FORMAT_V8;
}
