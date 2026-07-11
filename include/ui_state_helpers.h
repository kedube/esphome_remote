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

inline constexpr uint32_t UI_STATE_AUX_FORMAT_MASK = 3UL << 30;
inline constexpr uint32_t UI_STATE_AUX_FORMAT_V7 = 1UL << 31;
inline constexpr uint32_t UI_STATE_AUX_FORMAT_V8 = 3UL << 30;

// The V8 pack below masks each field to a fixed bit width; these asserts tie
// every width to the constant that bounds the corresponding value so a raised
// limit cannot silently wrap the restored index.
constexpr size_t max_favorite_list_entry_count() {
  size_t max_count = 0;
  for (size_t i = 0; i < FAVORITE_LIST_COUNT; i++) {
    if (FAVORITE_LISTS[i].count > max_count) {
      max_count = FAVORITE_LISTS[i].count;
    }
  }
  return max_count;
}

static_assert(MAX_PERSISTED_FAVORITE_LISTS + 2 <= 32, "current_menu_index is packed into 5 bits");
static_assert(max_favorite_list_entry_count() <= 64, "current_favorite_index is packed into 6 bits");
static_assert(NOTIFICATION_FEED_MAX_ITEMS <= 64, "selected_notification_index is packed into 6 bits");
static_assert(INFO_ITEM_COUNT <= 64, "selected_info_index is packed into 6 bits");
static_assert(REMOTE_MODE_COUNT <= 16, "current_mode is packed into 4 bits");
static_assert(REMOTE_SETTING_WATER_HEATER_AWAY < 64, "selected_setting_option is packed into 6 bits");
static_assert(ALARM_ARM_MODE_COUNT <= 8, "selected_alarm_arm_mode is packed into 3 bits");

inline PersistedUIStateData default_persisted_ui_state() {
  PersistedUIStateData data;
  data.contrast = 5;
  data.current_menu_index = 0;
  data.current_mode = REMOTE_MODE_INFO;
  return data;
}

inline PersistedUIStateData unpack_persisted_ui_state(uint64_t state, uint32_t aux_state) {
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

inline uint64_t pack_persisted_ui_state(const PersistedUIStateData &data) {
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

inline uint32_t pack_persisted_ui_state_aux(const PersistedUIStateData &data) {
  (void) data;
  return UI_STATE_AUX_FORMAT_V8;
}
