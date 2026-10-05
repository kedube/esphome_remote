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
  // UI_STATE_ENTITY_HASH_MASK bits of favorites_text_hash() of the selected
  // item's entity; 0 for none. Finds the item again when the favorite lists
  // have changed (see follow_saved_selection).
  uint32_t selected_entity_hash = 0;
};

inline constexpr uint32_t UI_STATE_AUX_FORMAT_MASK = 3UL << 30;
inline constexpr uint32_t UI_STATE_AUX_FORMAT_V7 = 1UL << 31;
inline constexpr uint32_t UI_STATE_AUX_FORMAT_V8 = 3UL << 30;
// The aux word's other bits hold the selected entity's hash. Firmware before
// it left them 0, and ignores them.
inline constexpr uint32_t UI_STATE_ENTITY_HASH_MASK = ~UI_STATE_AUX_FORMAT_MASK;
// V9 widens current_mode to 5 bits (V8 had 4, for up to 16 modes). It keeps
// V8's format bits and sets this marker in state bits 48-55, which V8 never
// set: format bits of 01 would match V6 state, whose top aux bits held the
// Info index.
inline constexpr uint64_t UI_STATE_V9_MARKER = uint64_t(0x59) << 48;
inline constexpr uint64_t UI_STATE_V9_MARKER_MASK = uint64_t(0xFF) << 48;

// The V9 pack below masks each field to a fixed bit width; these asserts tie
// every width to the constant that bounds the corresponding value so a raised
// limit cannot silently wrap the restored index. The favorite lists, from
// local_entities.h or Home Assistant, are held to FAVORITE_LIST_MAX_ITEMS
// and MAX_PERSISTED_FAVORITE_LISTS as they load.
static_assert(MAX_PERSISTED_FAVORITE_LISTS + 2 <= 32, "current_menu_index is packed into 5 bits");
static_assert(FAVORITE_LIST_MAX_ITEMS <= 64, "current_favorite_index is packed into 6 bits");
static_assert(NOTIFICATION_FEED_MAX_ITEMS <= 64, "selected_notification_index is packed into 6 bits");
static_assert(INFO_ITEM_COUNT <= 64, "selected_info_index is packed into 6 bits");
static_assert(REMOTE_MODE_COUNT <= 32, "current_mode is packed into 5 bits");
static_assert(REMOTE_SETTING_LAST < 64, "selected_setting_option is packed into 6 bits");
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
  if (format == UI_STATE_AUX_FORMAT_V8 && (state & UI_STATE_V9_MARKER_MASK) == UI_STATE_V9_MARKER) {
    PersistedUIStateData data;
    data.contrast = state & 0x0F;
    data.current_menu_index = (state >> 4) & 0x1F;
    data.current_favorite_index = (state >> 9) & 0x3F;
    data.selected_notification_index = (state >> 15) & 0x3F;
    data.selected_info_index = (state >> 21) & 0x3F;
    data.current_mode = (state >> 27) & 0x1F;
    data.selected_setting_option = (state >> 32) & 0x3F;
    data.selected_weather_detail_index = (state >> 38) & 0x0F;
    data.selected_alarm_arm_mode = (state >> 42) & 0x07;
    data.selected_entity_hash = aux_state & UI_STATE_ENTITY_HASH_MASK;
    return data;
  }

  // Saved by firmware before V9.
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
         (uint64_t(data.current_mode & 0x1F) << 27) |
         (uint64_t(data.selected_setting_option & 0x3F) << 32) |
         (uint64_t(data.selected_weather_detail_index & 0x0F) << 38) |
         (uint64_t(data.selected_alarm_arm_mode & 0x07) << 42) | UI_STATE_V9_MARKER;
}

inline uint32_t pack_persisted_ui_state_aux(const PersistedUIStateData &data) {
  return UI_STATE_AUX_FORMAT_V8 | (data.selected_entity_hash & UI_STATE_ENTITY_HASH_MASK);
}

// The hash persist_ui_state keeps of the selected item's entity.
inline uint32_t ui_state_entity_hash(const std::string &entity) {
  return entity.empty() ? 0 : favorites_text_hash(entity.c_str()) & UI_STATE_ENTITY_HASH_MASK;
}
