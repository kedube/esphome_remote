#pragma once

#include "entity_helpers_common.h"

struct PersistedUIStateData {
  int contrast = 0;
  int current_menu_index = 0;
  std::array<int, MAX_PERSISTED_FAVORITE_LISTS> favorite_indices{};
  int selected_notification_index = 0;
  int selected_info_index = 0;
};

static constexpr uint32_t UI_STATE_AUX_FORMAT_V7 = 1UL << 31;

static inline PersistedUIStateData default_persisted_ui_state() {
  PersistedUIStateData data;
  data.contrast = 5;
  data.current_menu_index = 0;
  return data;
}

static inline PersistedUIStateData unpack_persisted_ui_state(uint64_t state, uint32_t aux_state) {
  if ((aux_state & UI_STATE_AUX_FORMAT_V7) == 0) {
    return default_persisted_ui_state();
  }

  PersistedUIStateData data;
  data.contrast = state & 0x0F;
  data.current_menu_index = (state >> 4) & 0x0F;
  for (int i = 0; i < MAX_PERSISTED_FAVORITE_LISTS; i++) {
    data.favorite_indices[i] = (state >> (8 + i * 6)) & 0x3F;
  }
  data.selected_notification_index = aux_state & 0x3F;
  data.selected_info_index = (aux_state >> 6) & 0x3F;
  return data;
}

static inline uint64_t pack_persisted_ui_state(const PersistedUIStateData &data) {
  uint64_t state = uint64_t(data.contrast & 0x0F) |
                   (uint64_t(data.current_menu_index & 0x0F) << 4);
  for (int i = 0; i < MAX_PERSISTED_FAVORITE_LISTS; i++) {
    state |= uint64_t(data.favorite_indices[i] & 0x3F) << (8 + i * 6);
  }
  return state;
}

static inline uint32_t pack_persisted_ui_state_aux(const PersistedUIStateData &data) {
  return UI_STATE_AUX_FORMAT_V7 |
         (uint32_t(data.selected_notification_index & 0x3F)) |
         (uint32_t(data.selected_info_index & 0x3F) << 6);
}
