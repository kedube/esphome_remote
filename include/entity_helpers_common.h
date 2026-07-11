#pragma once

#include <ArduinoJson.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/json/json_util.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "esphome/core/string_ref.h"

enum RemoteMode {
  REMOTE_MODE_LIGHTS = 0,
  REMOTE_MODE_SWITCHES = 1,
  REMOTE_MODE_CLIMATE = 2,
  REMOTE_MODE_WATER_HEATERS = 3,
  REMOTE_MODE_HUMIDIFIERS = 4,
  REMOTE_MODE_FANS = 5,
  REMOTE_MODE_COVERS = 6,
  REMOTE_MODE_LOCKS = 7,
  REMOTE_MODE_MEDIA = 8,
  REMOTE_MODE_SENSORS = 9,
  REMOTE_MODE_AUTOMATION = 10,
  REMOTE_MODE_NOTIFICATIONS = 11,
  REMOTE_MODE_WEATHER = 12,
  REMOTE_MODE_INFO = 13,
  REMOTE_MODE_ALARMS = 14,
};

inline constexpr int REMOTE_MODE_COUNT = 15;
inline constexpr RemoteMode MENU_MODE_ORDER[] = {
    REMOTE_MODE_LIGHTS,
    REMOTE_MODE_SWITCHES,
    REMOTE_MODE_CLIMATE,
    REMOTE_MODE_WATER_HEATERS,
    REMOTE_MODE_HUMIDIFIERS,
    REMOTE_MODE_FANS,
    REMOTE_MODE_COVERS,
    REMOTE_MODE_LOCKS,
    REMOTE_MODE_MEDIA,
    REMOTE_MODE_SENSORS,
    REMOTE_MODE_AUTOMATION,
    REMOTE_MODE_ALARMS,
    REMOTE_MODE_WEATHER,
    REMOTE_MODE_NOTIFICATIONS,
    REMOTE_MODE_INFO,
};

struct EntityEntry {
  const char *name;
  const char *entity_id;
  const char *sources = nullptr;
};

struct FavoriteEntity {
    const char *name;
    const char *entity_id;
};

struct FavoriteList {
    const char *title;
    const FavoriteEntity *entries;
    size_t count;
};

template <size_t Count>
inline constexpr FavoriteList make_favorite_list(const char *title, const FavoriteEntity (&entries)[Count]) {
  return {title, entries, Count};
}

#include "local_entities.h"

inline constexpr size_t FAVORITE_LIST_COUNT = sizeof(FAVORITE_LISTS) / sizeof(FAVORITE_LISTS[0]);

inline constexpr int FAVORITE_LIST_COUNT_INT = static_cast<int>(FAVORITE_LIST_COUNT);
#ifndef REMOTE_MAX_PERSISTED_FAVORITE_LISTS
#define REMOTE_MAX_PERSISTED_FAVORITE_LISTS 16
#endif

inline constexpr int MAX_PERSISTED_FAVORITE_LISTS = REMOTE_MAX_PERSISTED_FAVORITE_LISTS;
static_assert(FAVORITE_LIST_COUNT_INT <= MAX_PERSISTED_FAVORITE_LISTS, "Too many favorite lists for persisted state");

inline constexpr bool cstr_eq_constexpr(const char *lhs, const char *rhs) {
  if (lhs == rhs) {
    return true;
  }
  if (lhs == nullptr || rhs == nullptr) {
    return false;
  }
  while (*lhs != '\0' && *rhs != '\0') {
    if (*lhs != *rhs) {
      return false;
    }
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

inline constexpr bool cstr_starts_with_constexpr(const char *value, const char *prefix) {
  if (value == nullptr || prefix == nullptr) {
    return false;
  }
  while (*prefix != '\0') {
    if (*value == '\0' || *value != *prefix) {
      return false;
    }
    ++value;
    ++prefix;
  }
  return true;
}

inline constexpr RemoteMode favorite_entity_mode_constexpr(const char *entity_id) {
  return cstr_starts_with_constexpr(entity_id, "light.")            ? REMOTE_MODE_LIGHTS :
         cstr_starts_with_constexpr(entity_id, "switch.")           ? REMOTE_MODE_SWITCHES :
         cstr_starts_with_constexpr(entity_id, "climate.")          ? REMOTE_MODE_CLIMATE :
         cstr_starts_with_constexpr(entity_id, "water_heater.")     ? REMOTE_MODE_WATER_HEATERS :
         cstr_starts_with_constexpr(entity_id, "humidifier.")       ? REMOTE_MODE_HUMIDIFIERS :
         cstr_starts_with_constexpr(entity_id, "fan.")              ? REMOTE_MODE_FANS :
         cstr_starts_with_constexpr(entity_id, "cover.")            ? REMOTE_MODE_COVERS :
         cstr_starts_with_constexpr(entity_id, "lock.")             ? REMOTE_MODE_LOCKS :
         cstr_starts_with_constexpr(entity_id, "media_player.")     ? REMOTE_MODE_MEDIA :
         (cstr_starts_with_constexpr(entity_id, "sensor.") ||
          cstr_starts_with_constexpr(entity_id, "binary_sensor."))  ? REMOTE_MODE_SENSORS :
         (cstr_starts_with_constexpr(entity_id, "automation.") ||
          cstr_starts_with_constexpr(entity_id, "script.") ||
          cstr_starts_with_constexpr(entity_id, "scene."))          ? REMOTE_MODE_AUTOMATION :
         cstr_starts_with_constexpr(entity_id, "alarm_control_panel.") ? REMOTE_MODE_ALARMS :
         cstr_starts_with_constexpr(entity_id, "weather.")          ? REMOTE_MODE_WEATHER :
                                                                        REMOTE_MODE_INFO;
}

inline constexpr bool favorite_entity_seen_earlier(size_t list_index, size_t entry_index, const char *entity_id) {
  for (size_t i = 0; i <= list_index && i < FAVORITE_LIST_COUNT; i++) {
    size_t limit = i == list_index ? entry_index : FAVORITE_LISTS[i].count;
    for (size_t j = 0; j < limit; j++) {
      if (cstr_eq_constexpr(FAVORITE_LISTS[i].entries[j].entity_id, entity_id)) {
        return true;
      }
    }
  }
  return false;
}

inline constexpr int count_unique_mode_entities(RemoteMode mode) {
  int count = 0;
  for (size_t i = 0; i < FAVORITE_LIST_COUNT; i++) {
    for (size_t j = 0; j < FAVORITE_LISTS[i].count; j++) {
      const FavoriteEntity &entry = FAVORITE_LISTS[i].entries[j];
      if (favorite_entity_mode_constexpr(entry.entity_id) != mode) {
        continue;
      }
      if (favorite_entity_seen_earlier(i, j, entry.entity_id)) {
        continue;
      }
      count++;
    }
  }
  return count;
}

template <size_t Count>
inline constexpr std::array<EntityEntry, Count> make_mode_entity_array(RemoteMode mode) {
  std::array<EntityEntry, Count> entities{};
  size_t out = 0;
  for (size_t i = 0; i < FAVORITE_LIST_COUNT; i++) {
    for (size_t j = 0; j < FAVORITE_LISTS[i].count; j++) {
      const FavoriteEntity &entry = FAVORITE_LISTS[i].entries[j];
      if (favorite_entity_mode_constexpr(entry.entity_id) != mode) {
        continue;
      }
      if (favorite_entity_seen_earlier(i, j, entry.entity_id)) {
        continue;
      }
      if (out < Count) {
        entities[out++] = {entry.name, entry.entity_id, nullptr};
      }
    }
  }
  return entities;
}

inline constexpr int LIGHT_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_LIGHTS);
inline constexpr int SWITCH_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_SWITCHES);
inline constexpr int CLIMATE_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_CLIMATE);
inline constexpr int WATER_HEATER_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_WATER_HEATERS);
inline constexpr int HUMIDIFIER_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_HUMIDIFIERS);
inline constexpr int FAN_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_FANS);
inline constexpr int COVER_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_COVERS);
inline constexpr int LOCK_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_LOCKS);
inline constexpr int MEDIA_PLAYER_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_MEDIA);
inline constexpr int SENSOR_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_SENSORS);
inline constexpr int AUTOMATION_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_AUTOMATION);
inline constexpr int ALARM_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_ALARMS);
inline constexpr int WEATHER_LIST_COUNT = count_unique_mode_entities(REMOTE_MODE_WEATHER);

inline constexpr auto LIGHT_LIST_STORAGE = make_mode_entity_array<LIGHT_LIST_COUNT>(REMOTE_MODE_LIGHTS);
inline constexpr auto SWITCH_LIST_STORAGE = make_mode_entity_array<SWITCH_LIST_COUNT>(REMOTE_MODE_SWITCHES);
inline constexpr auto CLIMATE_LIST_STORAGE = make_mode_entity_array<CLIMATE_LIST_COUNT>(REMOTE_MODE_CLIMATE);
inline constexpr auto WATER_HEATER_LIST_STORAGE = make_mode_entity_array<WATER_HEATER_LIST_COUNT>(REMOTE_MODE_WATER_HEATERS);
inline constexpr auto HUMIDIFIER_LIST_STORAGE = make_mode_entity_array<HUMIDIFIER_LIST_COUNT>(REMOTE_MODE_HUMIDIFIERS);
inline constexpr auto FAN_LIST_STORAGE = make_mode_entity_array<FAN_LIST_COUNT>(REMOTE_MODE_FANS);
inline constexpr auto COVER_LIST_STORAGE = make_mode_entity_array<COVER_LIST_COUNT>(REMOTE_MODE_COVERS);
inline constexpr auto LOCK_LIST_STORAGE = make_mode_entity_array<LOCK_LIST_COUNT>(REMOTE_MODE_LOCKS);
inline constexpr auto MEDIA_PLAYER_LIST_STORAGE = make_mode_entity_array<MEDIA_PLAYER_LIST_COUNT>(REMOTE_MODE_MEDIA);
inline constexpr auto SENSOR_LIST_STORAGE = make_mode_entity_array<SENSOR_LIST_COUNT>(REMOTE_MODE_SENSORS);
inline constexpr auto AUTOMATION_LIST_STORAGE = make_mode_entity_array<AUTOMATION_LIST_COUNT>(REMOTE_MODE_AUTOMATION);
inline constexpr auto ALARM_LIST_STORAGE = make_mode_entity_array<ALARM_LIST_COUNT>(REMOTE_MODE_ALARMS);
inline constexpr auto WEATHER_LIST_STORAGE = make_mode_entity_array<WEATHER_LIST_COUNT>(REMOTE_MODE_WEATHER);

inline constexpr const EntityEntry *LIGHT_LIST = LIGHT_LIST_STORAGE.data();
inline constexpr const EntityEntry *SWITCH_LIST = SWITCH_LIST_STORAGE.data();
inline constexpr const EntityEntry *CLIMATE_LIST = CLIMATE_LIST_STORAGE.data();
inline constexpr const EntityEntry *WATER_HEATER_LIST = WATER_HEATER_LIST_STORAGE.data();
inline constexpr const EntityEntry *HUMIDIFIER_LIST = HUMIDIFIER_LIST_STORAGE.data();
inline constexpr const EntityEntry *FAN_LIST = FAN_LIST_STORAGE.data();
inline constexpr const EntityEntry *COVER_LIST = COVER_LIST_STORAGE.data();
inline constexpr const EntityEntry *LOCK_LIST = LOCK_LIST_STORAGE.data();
inline constexpr const EntityEntry *MEDIA_PLAYER_LIST = MEDIA_PLAYER_LIST_STORAGE.data();
inline constexpr const EntityEntry *SENSOR_LIST = SENSOR_LIST_STORAGE.data();
inline constexpr const EntityEntry *AUTOMATION_LIST = AUTOMATION_LIST_STORAGE.data();
inline constexpr const EntityEntry *ALARM_LIST = ALARM_LIST_STORAGE.data();
inline constexpr const EntityEntry *WEATHER_LIST = WEATHER_LIST_STORAGE.data();
inline constexpr const char *INFO_ITEM_NAMES[] = {"Time & Date", "Wireless", "Network", "Device Name", "Battery", "Version"};
inline constexpr const char *INFO_ITEM_ENTITIES[] = {
    "info.date", "info.wireless", "info.network", "info.device_name", "info.battery", "info.version"};
inline constexpr int INFO_ITEM_COUNT = sizeof(INFO_ITEM_NAMES) / sizeof(INFO_ITEM_NAMES[0]);

#ifndef REMOTE_NOTIFICATION_FEED_MAX_ITEMS
#define REMOTE_NOTIFICATION_FEED_MAX_ITEMS 16
#endif

inline constexpr int NOTIFICATION_FEED_MAX_ITEMS = REMOTE_NOTIFICATION_FEED_MAX_ITEMS;

#ifndef NOTIFICATION_FEED_ENTITY
#define NOTIFICATION_FEED_ENTITY ""
#endif

#ifndef NOTIFICATION_FEED_ATTRIBUTE
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#endif

#ifndef NOTIFICATION_FEED_IDS_ATTRIBUTE
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#endif

#ifndef NOTIFICATION_FEED_SEPARATOR
#define NOTIFICATION_FEED_SEPARATOR "||"
#endif

inline int notification_mode_item_count();
inline std::string notification_mode_item_name(int idx);
inline std::string notification_mode_item_entity(int idx);
inline const std::string &notification_id_for_index(int idx);
inline bool notifications_mode_enabled();
inline const std::string &media_source_list_for_index(int idx);
inline const std::string &media_device_class_for_index(int idx);

// HA attribute payloads above this size are ignored by the list parsers; option
// lists this large would be unusable on the remote and parsing them risks
// exhausting the heap.
#ifndef REMOTE_HA_MAX_JSON_PAYLOAD_BYTES
#define REMOTE_HA_MAX_JSON_PAYLOAD_BYTES 8192
#endif

inline bool ha_api_ready() {
  return esphome::api::global_api_server != nullptr;
}

inline bool ha_state_missing(const char *data, size_t len) {
  if (len == 0) {
    return true;
  }
  return (len == 7 && memcmp(data, "unknown", 7) == 0) ||
         (len == 11 && memcmp(data, "unavailable", 11) == 0) ||
         (len == 4 && memcmp(data, "None", 4) == 0);
}

inline bool ha_state_missing(const std::string &value) {
  return ha_state_missing(value.data(), value.size());
}

inline bool ha_state_missing(esphome::StringRef state) {
  return ha_state_missing(state.c_str(), state.size());
}

// Assigns into an existing string so its capacity is reused instead of
// allocating a temporary on every state callback.
inline void ha_assign(std::string &target, esphome::StringRef state) {
  target.assign(state.c_str(), state.size());
}

inline void ha_assign_state_or_unknown(std::string &target, esphome::StringRef state) {
  if (ha_state_missing(state)) {
    target = "unknown";
  } else {
    target.assign(state.c_str(), state.size());
  }
}

inline float ha_parse_float(esphome::StringRef state) {
  if (ha_state_missing(state)) {
    return NAN;
  }
  char buffer[32];
  size_t len = state.size() < sizeof(buffer) - 1 ? state.size() : sizeof(buffer) - 1;
  memcpy(buffer, state.c_str(), len);
  buffer[len] = '\0';
  char *end = nullptr;
  float result = strtof(buffer, &end);
  return end != buffer ? result : NAN;
}

inline std::string trim_copy(const std::string &value) {
  size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

inline bool ha_payload_looks_like_json(const char *data, size_t len) {
  size_t i = 0;
  while (i < len && (data[i] == ' ' || data[i] == '\t' || data[i] == '\r' || data[i] == '\n')) {
    i++;
  }
  return i < len && (data[i] == '[' || data[i] == '{');
}

inline bool ha_payload_looks_like_json(const std::string &value) {
  return ha_payload_looks_like_json(value.data(), value.size());
}

inline bool ha_payload_looks_like_json(esphome::StringRef state) {
  return ha_payload_looks_like_json(state.c_str(), state.size());
}

// Parses a JSON array attribute payload (e.g. effect_list, preset_modes) and
// joins its trimmed items into target separated by '|'. Clears target when the
// payload is missing, oversized, or not a JSON array.
inline void ha_store_joined_list(std::string &target, esphome::StringRef state) {
  target.clear();
  if (ha_state_missing(state) || state.size() > REMOTE_HA_MAX_JSON_PAYLOAD_BYTES) {
    return;
  }
  if (!ha_payload_looks_like_json(state)) {
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, state.c_str(), state.size()) != DeserializationError::Ok || !doc.is<JsonArray>()) {
    return;
  }

  for (JsonVariant value : doc.as<JsonArray>()) {
    const char *item = value.as<const char *>();
    if (item == nullptr) {
      continue;
    }
    size_t len = strlen(item);
    while (len > 0 && (item[0] == ' ' || item[0] == '\t' || item[0] == '\r' || item[0] == '\n')) {
      item++;
      len--;
    }
    while (len > 0 && (item[len - 1] == ' ' || item[len - 1] == '\t' || item[len - 1] == '\r' || item[len - 1] == '\n')) {
      len--;
    }
    if (len == 0) {
      continue;
    }
    if (!target.empty()) {
      target += '|';
    }
    target.append(item, len);
  }
}

inline void remote_state_label_to_buffer(
    const std::string &raw, char *buffer, size_t buffer_size, const char *fallback = "SYNCING") {
  if (buffer == nullptr || buffer_size == 0) {
    return;
  }

  size_t write_idx = 0;
  for (char ch : raw) {
    if (write_idx + 1 >= buffer_size) {
      break;
    }
    if (ch >= 'a' && ch <= 'z') {
      buffer[write_idx++] = ch - 'a' + 'A';
    } else if (ch == '_') {
      buffer[write_idx++] = ' ';
    } else {
      buffer[write_idx++] = ch;
    }
  }
  buffer[write_idx] = '\0';

  if (write_idx == 0 || strcmp(buffer, "UNKNOWN") == 0) {
    snprintf(buffer, buffer_size, "%s", fallback != nullptr ? fallback : "");
  }
}

// ASCII uppercase into a fixed buffer; avoids the temporary std::string that
// str_upper_case() allocates when the result is only compared and discarded.
inline void str_upper_to_buffer(const std::string &raw, char *buffer, size_t buffer_size) {
  if (buffer == nullptr || buffer_size == 0) {
    return;
  }
  size_t write_idx = 0;
  for (char ch : raw) {
    if (write_idx + 1 >= buffer_size) {
      break;
    }
    buffer[write_idx++] = (ch >= 'a' && ch <= 'z') ? ch - 'a' + 'A' : ch;
  }
  buffer[write_idx] = '\0';
}

inline int clamp_percent_value(float value, float scale = 1.0f, int min_value = 0) {
  if (std::isnan(value)) {
    return min_value;
  }
  int pct = (int) roundf(value * scale);
  if (pct < min_value) pct = min_value;
  if (pct > 100) pct = 100;
  return pct;
}

inline std::string lock_operation_feedback_for_state(const std::string &state) {
  if (state == "jammed") {
    return "JAMMED";
  }
  if (state == "locking") {
    return "LOCKING...";
  }
  if (state == "unlocking") {
    return "UNLOCKING...";
  }
  return "";
}

inline bool cover_state_matches_expected(const std::string &state, float position, const std::string &expected_state) {
  const bool reached_open = state == "open" || (!std::isnan(position) && position >= 99.0f);
  const bool reached_closed = state == "closed" || (!std::isnan(position) && position <= 1.0f);
  return (expected_state == "open" && reached_open) || (expected_state == "closed" && reached_closed);
}

inline std::string cover_operation_feedback_for_state(const std::string &state, float position) {
  if (state == "opening") {
    return "OPENING...";
  }
  if (state == "closing") {
    return "CLOSING...";
  }
  if (!std::isnan(position) && position > 0.0f && position < 100.0f) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "OPEN %d%%", (int) roundf(position));
    return std::string(buffer);
  }
  return "";
}

inline std::string alarm_operation_feedback_for_state(const std::string &state) {
  if (state == "arming" || state == "pending") {
    return "ARMING...";
  }
  if (state == "disarming") {
    return "DISARMING...";
  }
  if (state == "triggered") {
    return "TRIGGERED";
  }
  return "";
}

// Visits each trimmed, non-empty '|'-separated item as (offset, length) into
// source_list without allocating. The callback returns false to stop early.
template <typename Fn>
inline void for_each_delimited_option(const std::string &source_list, Fn fn) {
  size_t start = 0;
  while (start <= source_list.size()) {
    size_t end = source_list.find('|', start);
    size_t item_end = end == std::string::npos ? source_list.size() : end;
    size_t begin = start;
    while (begin < item_end && (source_list[begin] == ' ' || source_list[begin] == '\t' ||
                                source_list[begin] == '\r' || source_list[begin] == '\n')) {
      begin++;
    }
    while (item_end > begin && (source_list[item_end - 1] == ' ' || source_list[item_end - 1] == '\t' ||
                                source_list[item_end - 1] == '\r' || source_list[item_end - 1] == '\n')) {
      item_end--;
    }
    if (item_end > begin && !fn(begin, item_end - begin)) {
      return;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
}

inline int delimited_option_count(const std::string &source_list) {
  int count = 0;
  for_each_delimited_option(source_list, [&](size_t, size_t) {
    count++;
    return true;
  });
  return count;
}

inline std::string delimited_option_at(const std::string &source_list, int option_idx) {
  std::string result;
  if (option_idx < 0) {
    return result;
  }
  int current_idx = 0;
  for_each_delimited_option(source_list, [&](size_t offset, size_t len) {
    if (current_idx++ == option_idx) {
      result = source_list.substr(offset, len);
      return false;
    }
    return true;
  });
  return result;
}

inline std::string previous_delimited_option(const std::string &source_list, const std::string &current_value) {
  std::string current = trim_copy(current_value);
  size_t prev_offset = 0, prev_len = 0, last_offset = 0, last_len = 0;
  bool have_prev = false, matched_first = false, matched = false;
  for_each_delimited_option(source_list, [&](size_t offset, size_t len) {
    if (!matched && source_list.compare(offset, len, current) == 0) {
      matched = true;
      if (!have_prev) {
        matched_first = true;  // wrap around: keep scanning for the last item
      }
    }
    if (!matched || matched_first) {
      last_offset = offset;
      last_len = len;
      if (!matched) {
        prev_offset = offset;
        prev_len = len;
        have_prev = true;
      }
    }
    return true;
  });
  if (matched && !matched_first && have_prev) {
    return source_list.substr(prev_offset, prev_len);
  }
  // Wrapped past the first item, or no match: fall back to the last item.
  return last_len > 0 ? source_list.substr(last_offset, last_len) : "";
}

inline std::string next_delimited_option(const std::string &source_list, const std::string &current_value) {
  std::string current = trim_copy(current_value);
  std::string first, result;
  bool matched = false;
  for_each_delimited_option(source_list, [&](size_t offset, size_t len) {
    if (first.empty()) {
      first = source_list.substr(offset, len);
    }
    if (matched) {
      result = source_list.substr(offset, len);
      return false;
    }
    if (source_list.compare(offset, len, current) == 0) {
      matched = true;
    }
    return true;
  });
  if (!result.empty()) {
    return result;
  }
  // Current item was last (wrap around) or not found: start from the first item.
  return first;
}

inline bool notifications_mode_enabled() {
  return NOTIFICATION_FEED_ENTITY[0] != '\0';
}

inline std::string media_configured_source_list_for_index(int idx) {
  if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
    return "";
  }
  const char *sources = MEDIA_PLAYER_LIST[idx].sources;
  return (sources == nullptr) ? "" : std::string(sources);
}

inline std::string active_media_source_list_for_index(int idx) {
  std::string tracked_sources = media_source_list_for_index(idx);
  std::string device_class = media_device_class_for_index(idx);
  if ((device_class == "tv" || device_class == "receiver") && !tracked_sources.empty()) {
    return tracked_sources;
  }
  return media_configured_source_list_for_index(idx);
}

inline int media_source_option_count(int idx) {
  return delimited_option_count(active_media_source_list_for_index(idx));
}

inline std::string media_source_option_at(int idx, int option_idx) {
  return delimited_option_at(active_media_source_list_for_index(idx), option_idx);
}

inline std::string next_media_source_for_index(int idx, const std::string &current_source) {
  const std::string source_list = active_media_source_list_for_index(idx);
  return next_delimited_option(source_list, current_source);
}

inline std::string previous_media_source_for_index(int idx, const std::string &current_source) {
  const std::string source_list = active_media_source_list_for_index(idx);
  return previous_delimited_option(source_list, current_source);
}

inline bool entity_id_matches_domain(const char *entity_id, const char *domain) {
  if (entity_id == nullptr || domain == nullptr) {
    return false;
  }
  size_t domain_len = strlen(domain);
  return strncmp(entity_id, domain, domain_len) == 0 && entity_id[domain_len] == '.';
}

template <typename Entity>
inline void validate_entity_list(
    const char *label, const Entity *entities, int count, const char *primary_domain,
    const char *secondary_domain = nullptr, const char *tertiary_domain = nullptr) {
  for (int i = 0; i < count; i++) {
    const char *name = entities[i].name;
    const char *entity_id = entities[i].entity_id;
    if (name == nullptr || name[0] == '\0') {
      ESP_LOGW("remote_config", "%s[%d] is missing a display name", label, i);
    }
    if (entity_id == nullptr || entity_id[0] == '\0') {
      ESP_LOGW("remote_config", "%s[%d] is missing an entity_id", label, i);
      continue;
    }
    bool valid_domain = entity_id_matches_domain(entity_id, primary_domain) ||
                        entity_id_matches_domain(entity_id, secondary_domain) ||
                        entity_id_matches_domain(entity_id, tertiary_domain);
    if (!valid_domain) {
      ESP_LOGW("remote_config", "%s[%d] has unexpected domain: %s", label, i, entity_id);
    }
  }
}

inline void validate_notification_config() {
  if (!notifications_mode_enabled()) {
    return;
  }
  if (!entity_id_matches_domain(NOTIFICATION_FEED_ENTITY, "sensor")) {
    ESP_LOGW("remote_config", "NOTIFICATION_FEED_ENTITY should point to a sensor.* entity: %s", NOTIFICATION_FEED_ENTITY);
  }
}

inline void validate_remote_configuration() {
  validate_entity_list("LIGHT_LIST", LIGHT_LIST, LIGHT_LIST_COUNT, "light");
  validate_entity_list("SWITCH_LIST", SWITCH_LIST, SWITCH_LIST_COUNT, "switch");
  validate_entity_list("CLIMATE_LIST", CLIMATE_LIST, CLIMATE_LIST_COUNT, "climate");
  validate_entity_list("HUMIDIFIER_LIST", HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, "humidifier");
  validate_entity_list("FAN_LIST", FAN_LIST, FAN_LIST_COUNT, "fan");
  validate_entity_list("COVER_LIST", COVER_LIST, COVER_LIST_COUNT, "cover");
  validate_entity_list("LOCK_LIST", LOCK_LIST, LOCK_LIST_COUNT, "lock");
  validate_entity_list("MEDIA_PLAYER_LIST", MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, "media_player");
  validate_entity_list("SENSOR_LIST", SENSOR_LIST, SENSOR_LIST_COUNT, "sensor", "binary_sensor");
  validate_entity_list("AUTOMATION_LIST", AUTOMATION_LIST, AUTOMATION_LIST_COUNT, "automation", "script", "scene");
  validate_entity_list("ALARM_LIST", ALARM_LIST, ALARM_LIST_COUNT, "alarm_control_panel");
  validate_entity_list("WEATHER_LIST", WEATHER_LIST, WEATHER_LIST_COUNT, "weather");
  validate_notification_config();
}

template <typename Entity>
inline int find_entity_index(const Entity *entities, int count, const std::string &entity_id) {
  for (int i = 0; i < count; i++) {
    if (entity_id == entities[i].entity_id) {
      return i;
    }
  }
  return -1;
}

template <typename Entity>
inline const char *indexed_entity_name_cstr(const Entity *entities, int count, int idx) {
  return (idx >= 0 && idx < count) ? entities[idx].name : "";
}

template <typename Entity>
inline const char *indexed_entity_id_cstr(const Entity *entities, int count, int idx) {
  return (idx >= 0 && idx < count) ? entities[idx].entity_id : "";
}

template <typename Entity>
inline int index_of_entity_id(const Entity *entities, int count, const char *entity_id) {
  if (entity_id == nullptr || entity_id[0] == '\0') {
    return -1;
  }
  for (int i = 0; i < count; i++) {
    if (strcmp(entities[i].entity_id, entity_id) == 0) {
      return i;
    }
  }
  return -1;
}

inline const char *indexed_value_cstr(const char *const *values, int count, int idx) {
  return (idx >= 0 && idx < count) ? values[idx] : "";
}

enum RemoteSettingOption {
  REMOTE_SETTING_NONE = 0,
  REMOTE_SETTING_LIGHT_DIMMER,
  REMOTE_SETTING_LIGHT_EFFECT,
  REMOTE_SETTING_CLIMATE_LOW,
  REMOTE_SETTING_CLIMATE_HIGH,
  REMOTE_SETTING_CLIMATE_TARGET,
  REMOTE_SETTING_CLIMATE_FAN,
  REMOTE_SETTING_CLIMATE_HUMIDITY,
  REMOTE_SETTING_CLIMATE_PRESETS,
  REMOTE_SETTING_CLIMATE_HVAC_MODE,
  REMOTE_SETTING_CLIMATE_ACTION,
  REMOTE_SETTING_CLIMATE_STATE,
  REMOTE_SETTING_HUMIDIFIER_HUMIDITY,
  REMOTE_SETTING_HUMIDIFIER_MODE,
  REMOTE_SETTING_HUMIDIFIER_ACTION,
  REMOTE_SETTING_HUMIDIFIER_STATE,
  REMOTE_SETTING_FAN_SPEED,
  REMOTE_SETTING_FAN_PRESETS,
  REMOTE_SETTING_FAN_OSCILLATE,
  REMOTE_SETTING_FAN_DIRECTION,
  REMOTE_SETTING_COVER_POSITION,
  REMOTE_SETTING_COVER_TILT,
  REMOTE_SETTING_MEDIA_SELECT,
  REMOTE_SETTING_MEDIA_VOLUME,
  REMOTE_SETTING_MEDIA_SHUFFLE,
  REMOTE_SETTING_MEDIA_CHANNEL,
  REMOTE_SETTING_MEDIA_SOURCE,
  REMOTE_SETTING_MEDIA_REPEAT,
  REMOTE_SETTING_MEDIA_SOUND,
  REMOTE_SETTING_MEDIA_STATE,
  REMOTE_SETTING_ALARM_STATE,
  REMOTE_SETTING_NOTIFICATION_MESSAGES,
  REMOTE_SETTING_WEATHER_CONDITIONS,
  REMOTE_SETTING_WEATHER_HUMIDITY,
  REMOTE_SETTING_WEATHER_WIND_SPEED,
  REMOTE_SETTING_WEATHER_WIND_BEARING,
  REMOTE_SETTING_WEATHER_WIND_GUST,
  REMOTE_SETTING_WEATHER_PRESSURE,
  REMOTE_SETTING_WEATHER_PRECIPITATION,
  REMOTE_SETTING_WEATHER_CLOUD_COVERAGE,
  REMOTE_SETTING_WEATHER_UV_INDEX,
  REMOTE_SETTING_WEATHER_DEW_POINT,
  REMOTE_SETTING_WEATHER_APPARENT_TEMP,
  REMOTE_SETTING_WEATHER_HIGH_TEMP,
  REMOTE_SETTING_WEATHER_LOW_TEMP,
  REMOTE_SETTING_WATER_HEATER_TARGET,
  REMOTE_SETTING_WATER_HEATER_MODE,
  REMOTE_SETTING_WATER_HEATER_AWAY,
};

inline const char *remote_setting_option_label(RemoteSettingOption option) {
  switch (option) {
    case REMOTE_SETTING_LIGHT_DIMMER:
      return "DIMMER";
    case REMOTE_SETTING_LIGHT_EFFECT:
      return "EFFECT";
    case REMOTE_SETTING_CLIMATE_LOW:
      return "LOW";
    case REMOTE_SETTING_CLIMATE_HIGH:
      return "HIGH";
    case REMOTE_SETTING_CLIMATE_TARGET:
      return "TARGET";
    case REMOTE_SETTING_CLIMATE_FAN:
      return "FAN";
    case REMOTE_SETTING_CLIMATE_HUMIDITY:
      return "HUMIDITY";
    case REMOTE_SETTING_CLIMATE_PRESETS:
      return "PRESETS";
    case REMOTE_SETTING_CLIMATE_HVAC_MODE:
      return "HVAC MODE";
    case REMOTE_SETTING_CLIMATE_ACTION:
      return "STATUS";
    case REMOTE_SETTING_CLIMATE_STATE:
      return "MODE";
    case REMOTE_SETTING_HUMIDIFIER_HUMIDITY:
      return "HUMIDITY";
    case REMOTE_SETTING_HUMIDIFIER_MODE:
      return "MODE";
    case REMOTE_SETTING_HUMIDIFIER_ACTION:
      return "STATUS";
    case REMOTE_SETTING_HUMIDIFIER_STATE:
      return "STATUS";
    case REMOTE_SETTING_FAN_SPEED:
      return "SPEED";
    case REMOTE_SETTING_FAN_PRESETS:
      return "PRESETS";
    case REMOTE_SETTING_FAN_OSCILLATE:
      return "OSCILLATE";
    case REMOTE_SETTING_FAN_DIRECTION:
      return "DIRECTION";
    case REMOTE_SETTING_COVER_POSITION:
      return "POSITION";
    case REMOTE_SETTING_COVER_TILT:
      return "TILT";
    case REMOTE_SETTING_MEDIA_SELECT:
      return "SELECT";
    case REMOTE_SETTING_MEDIA_VOLUME:
      return "VOLUME";
    case REMOTE_SETTING_MEDIA_SHUFFLE:
      return "SHUFFLE";
    case REMOTE_SETTING_MEDIA_CHANNEL:
      return "CHANNEL";
    case REMOTE_SETTING_MEDIA_SOURCE:
      return "SOURCE";
    case REMOTE_SETTING_MEDIA_REPEAT:
      return "REPEAT";
    case REMOTE_SETTING_MEDIA_SOUND:
      return "SOUND";
    case REMOTE_SETTING_MEDIA_STATE:
      return "STATE";
    case REMOTE_SETTING_ALARM_STATE:
      return "MODE";
    case REMOTE_SETTING_NOTIFICATION_MESSAGES:
      return "MESSAGES";
    case REMOTE_SETTING_WEATHER_CONDITIONS:
      return "CONDITIONS";
    case REMOTE_SETTING_WEATHER_HUMIDITY:
      return "HUMIDITY";
    case REMOTE_SETTING_WEATHER_WIND_SPEED:
      return "WIND SPEED";
    case REMOTE_SETTING_WEATHER_WIND_BEARING:
      return "WIND DIR";
    case REMOTE_SETTING_WEATHER_WIND_GUST:
      return "WIND GUST";
    case REMOTE_SETTING_WEATHER_PRESSURE:
      return "PRESSURE";
    case REMOTE_SETTING_WEATHER_PRECIPITATION:
      return "PRECIP";
    case REMOTE_SETTING_WEATHER_CLOUD_COVERAGE:
      return "CLOUD COV";
    case REMOTE_SETTING_WEATHER_UV_INDEX:
      return "UV INDEX";
    case REMOTE_SETTING_WEATHER_DEW_POINT:
      return "DEW POINT";
    case REMOTE_SETTING_WEATHER_APPARENT_TEMP:
      return "FEELS LIKE";
    case REMOTE_SETTING_WEATHER_HIGH_TEMP:
      return "HIGH TEMP";
    case REMOTE_SETTING_WEATHER_LOW_TEMP:
      return "LOW TEMP";
    case REMOTE_SETTING_WATER_HEATER_TARGET:
      return "TARGET";
    case REMOTE_SETTING_WATER_HEATER_MODE:
      return "MODE";
    case REMOTE_SETTING_WATER_HEATER_AWAY:
      return "AWAY";
    case REMOTE_SETTING_NONE:
    default:
      return "";
  }
}

inline const char *remote_setting_left_icon(RemoteSettingOption option) {
  switch (option) {
    case REMOTE_SETTING_LIGHT_DIMMER:
    case REMOTE_SETTING_CLIMATE_HUMIDITY:
    case REMOTE_SETTING_HUMIDIFIER_HUMIDITY:
    case REMOTE_SETTING_FAN_SPEED:
    case REMOTE_SETTING_FAN_DIRECTION:
    case REMOTE_SETTING_COVER_POSITION:
    case REMOTE_SETTING_COVER_TILT:
    case REMOTE_SETTING_WATER_HEATER_AWAY:
    case REMOTE_SETTING_CLIMATE_LOW:
    case REMOTE_SETTING_CLIMATE_HIGH:
    case REMOTE_SETTING_CLIMATE_TARGET:
    case REMOTE_SETTING_WATER_HEATER_TARGET:
      return "\ue15b";
    case REMOTE_SETTING_MEDIA_SELECT:
      return "\ue045";
    case REMOTE_SETTING_MEDIA_CHANNEL:
      return "\uead0";
    case REMOTE_SETTING_MEDIA_VOLUME:
      return "\ue04d";
    case REMOTE_SETTING_LIGHT_EFFECT:
    case REMOTE_SETTING_CLIMATE_FAN:
    case REMOTE_SETTING_CLIMATE_PRESETS:
    case REMOTE_SETTING_CLIMATE_HVAC_MODE:
    case REMOTE_SETTING_HUMIDIFIER_MODE:
    case REMOTE_SETTING_FAN_PRESETS:
    case REMOTE_SETTING_FAN_OSCILLATE:
    case REMOTE_SETTING_MEDIA_SOURCE:
    case REMOTE_SETTING_MEDIA_SHUFFLE:
    case REMOTE_SETTING_MEDIA_REPEAT:
    case REMOTE_SETTING_MEDIA_SOUND:
    case REMOTE_SETTING_MEDIA_STATE:
    case REMOTE_SETTING_ALARM_STATE:
    case REMOTE_SETTING_NOTIFICATION_MESSAGES:
    case REMOTE_SETTING_WATER_HEATER_MODE:
    case REMOTE_SETTING_WEATHER_CONDITIONS:
    case REMOTE_SETTING_WEATHER_HUMIDITY:
    case REMOTE_SETTING_WEATHER_WIND_SPEED:
    case REMOTE_SETTING_WEATHER_WIND_BEARING:
    case REMOTE_SETTING_WEATHER_WIND_GUST:
    case REMOTE_SETTING_WEATHER_PRESSURE:
    case REMOTE_SETTING_WEATHER_PRECIPITATION:
    case REMOTE_SETTING_WEATHER_CLOUD_COVERAGE:
    case REMOTE_SETTING_WEATHER_UV_INDEX:
    case REMOTE_SETTING_WEATHER_DEW_POINT:
    case REMOTE_SETTING_WEATHER_APPARENT_TEMP:
    case REMOTE_SETTING_WEATHER_HIGH_TEMP:
    case REMOTE_SETTING_WEATHER_LOW_TEMP:
      return "\ueac3";
    case REMOTE_SETTING_CLIMATE_ACTION:
    case REMOTE_SETTING_HUMIDIFIER_ACTION:
    case REMOTE_SETTING_HUMIDIFIER_STATE:
    case REMOTE_SETTING_NONE:
    default:
      return "";
  }
}

inline const char *remote_setting_right_icon(RemoteSettingOption option) {
  switch (option) {
    case REMOTE_SETTING_LIGHT_DIMMER:
    case REMOTE_SETTING_CLIMATE_HUMIDITY:
    case REMOTE_SETTING_HUMIDIFIER_HUMIDITY:
    case REMOTE_SETTING_FAN_SPEED:
    case REMOTE_SETTING_FAN_DIRECTION:
    case REMOTE_SETTING_COVER_POSITION:
    case REMOTE_SETTING_COVER_TILT:
    case REMOTE_SETTING_WATER_HEATER_AWAY:
    case REMOTE_SETTING_CLIMATE_LOW:
    case REMOTE_SETTING_CLIMATE_HIGH:
    case REMOTE_SETTING_CLIMATE_TARGET:
    case REMOTE_SETTING_WATER_HEATER_TARGET:
      return "\ue145";
    case REMOTE_SETTING_MEDIA_SELECT:
      return "\ue044";
    case REMOTE_SETTING_MEDIA_CHANNEL:
      return "\ueacf";
    case REMOTE_SETTING_MEDIA_VOLUME:
      return "\ue050";
    case REMOTE_SETTING_LIGHT_EFFECT:
    case REMOTE_SETTING_CLIMATE_FAN:
    case REMOTE_SETTING_CLIMATE_PRESETS:
    case REMOTE_SETTING_CLIMATE_HVAC_MODE:
    case REMOTE_SETTING_HUMIDIFIER_MODE:
    case REMOTE_SETTING_FAN_PRESETS:
    case REMOTE_SETTING_FAN_OSCILLATE:
    case REMOTE_SETTING_MEDIA_SOURCE:
    case REMOTE_SETTING_MEDIA_SHUFFLE:
    case REMOTE_SETTING_MEDIA_REPEAT:
    case REMOTE_SETTING_MEDIA_SOUND:
    case REMOTE_SETTING_MEDIA_STATE:
    case REMOTE_SETTING_ALARM_STATE:
    case REMOTE_SETTING_NOTIFICATION_MESSAGES:
    case REMOTE_SETTING_WATER_HEATER_MODE:
    case REMOTE_SETTING_WEATHER_CONDITIONS:
    case REMOTE_SETTING_WEATHER_HUMIDITY:
    case REMOTE_SETTING_WEATHER_WIND_SPEED:
    case REMOTE_SETTING_WEATHER_WIND_BEARING:
    case REMOTE_SETTING_WEATHER_WIND_GUST:
    case REMOTE_SETTING_WEATHER_PRESSURE:
    case REMOTE_SETTING_WEATHER_PRECIPITATION:
    case REMOTE_SETTING_WEATHER_CLOUD_COVERAGE:
    case REMOTE_SETTING_WEATHER_UV_INDEX:
    case REMOTE_SETTING_WEATHER_DEW_POINT:
    case REMOTE_SETTING_WEATHER_APPARENT_TEMP:
    case REMOTE_SETTING_WEATHER_HIGH_TEMP:
    case REMOTE_SETTING_WEATHER_LOW_TEMP:      
      return "\ueac9";
    case REMOTE_SETTING_CLIMATE_ACTION:
    case REMOTE_SETTING_HUMIDIFIER_ACTION:
    case REMOTE_SETTING_HUMIDIFIER_STATE:
    case REMOTE_SETTING_NONE:
    default:
      return "";
  }
}

inline const char *mode_title(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return "LIGHTS";
    case REMOTE_MODE_SWITCHES:
      return "SWITCHES";
    case REMOTE_MODE_CLIMATE:
      return "CLIMATE";
    case REMOTE_MODE_WATER_HEATERS:
      return "WATER HEATERS";
    case REMOTE_MODE_HUMIDIFIERS:
      return "HUMIDIFIERS";
    case REMOTE_MODE_FANS:
      return "FANS";
    case REMOTE_MODE_COVERS:
      return "COVERS";
    case REMOTE_MODE_LOCKS:
      return "DOOR LOCKS";
    case REMOTE_MODE_MEDIA:
      return "MEDIA";
    case REMOTE_MODE_SENSORS:
      return "SENSORS";
    case REMOTE_MODE_AUTOMATION:
      return "AUTOMATIONS";
    case REMOTE_MODE_ALARMS:
      return "ALARMS";
    case REMOTE_MODE_NOTIFICATIONS:
      return "NOTIFICATIONS";
    case REMOTE_MODE_WEATHER:
      return "WEATHER";
    case REMOTE_MODE_INFO:
      return "INFO";
    default:
      return "MODE";
  }
}

inline const char *mode_icon(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return "\ue90f";
    case REMOTE_MODE_SWITCHES:
      return "\ue1f4";
    case REMOTE_MODE_CLIMATE:
      return "\uf076";
    case REMOTE_MODE_WATER_HEATERS:
      return "\uf16a";
    case REMOTE_MODE_HUMIDIFIERS:
      return "\uf165";
    case REMOTE_MODE_FANS:
      return "\uf168";
    case REMOTE_MODE_COVERS:
      return "\ue286";
    case REMOTE_MODE_LOCKS:
      return "\ue897";
    case REMOTE_MODE_MEDIA:
      return "\uf4f2";
    case REMOTE_MODE_SENSORS:
      return "\uf556";
    case REMOTE_MODE_AUTOMATION:
      return "\ue88a";
    case REMOTE_MODE_ALARMS:
      return "\ue7f7";
    case REMOTE_MODE_NOTIFICATIONS:
      return "\ue158";
    case REMOTE_MODE_WEATHER:
      return "\ue81a";
    case REMOTE_MODE_INFO:
      return "\ue88e";
    default:
      return "\ue90f";
  }
}

inline RemoteMode favorite_entity_mode(const char *entity_id) {
  return favorite_entity_mode_constexpr(entity_id);
}

inline int favorite_entity_mode_index(RemoteMode mode, const char *entity_id) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return index_of_entity_id(LIGHT_LIST, LIGHT_LIST_COUNT, entity_id);
    case REMOTE_MODE_SWITCHES:
      return index_of_entity_id(SWITCH_LIST, SWITCH_LIST_COUNT, entity_id);
    case REMOTE_MODE_CLIMATE:
      return index_of_entity_id(CLIMATE_LIST, CLIMATE_LIST_COUNT, entity_id);
    case REMOTE_MODE_WATER_HEATERS:
      return index_of_entity_id(WATER_HEATER_LIST, WATER_HEATER_LIST_COUNT, entity_id);
    case REMOTE_MODE_HUMIDIFIERS:
      return index_of_entity_id(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, entity_id);
    case REMOTE_MODE_FANS:
      return index_of_entity_id(FAN_LIST, FAN_LIST_COUNT, entity_id);
    case REMOTE_MODE_COVERS:
      return index_of_entity_id(COVER_LIST, COVER_LIST_COUNT, entity_id);
    case REMOTE_MODE_LOCKS:
      return index_of_entity_id(LOCK_LIST, LOCK_LIST_COUNT, entity_id);
    case REMOTE_MODE_MEDIA:
      return index_of_entity_id(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, entity_id);
    case REMOTE_MODE_SENSORS:
      return index_of_entity_id(SENSOR_LIST, SENSOR_LIST_COUNT, entity_id);
    case REMOTE_MODE_AUTOMATION:
      return index_of_entity_id(AUTOMATION_LIST, AUTOMATION_LIST_COUNT, entity_id);
    case REMOTE_MODE_ALARMS:
      return index_of_entity_id(ALARM_LIST, ALARM_LIST_COUNT, entity_id);
    case REMOTE_MODE_WEATHER:
      return index_of_entity_id(WEATHER_LIST, WEATHER_LIST_COUNT, entity_id);
    default:
      return -1;
  }
}

inline int favorite_list_item_count(int list_index) {
  return (list_index >= 0 && list_index < FAVORITE_LIST_COUNT_INT) ? static_cast<int>(FAVORITE_LISTS[list_index].count) : 0;
}

inline bool favorite_list_is_available(int list_index) {
  return favorite_list_item_count(list_index) > 0;
}

inline const char *favorite_list_title(int list_index) {
  return (list_index >= 0 && list_index < FAVORITE_LIST_COUNT_INT) ? FAVORITE_LISTS[list_index].title : "FAVORITES";
}

inline const FavoriteEntity *favorite_list_entry(int list_index, int item_index) {
  if (list_index < 0 || list_index >= FAVORITE_LIST_COUNT_INT) {
    return nullptr;
  }
  const FavoriteList &list = FAVORITE_LISTS[list_index];
  if (item_index < 0 || item_index >= static_cast<int>(list.count)) {
    return nullptr;
  }
  return &list.entries[item_index];
}

inline std::array<int, FAVORITE_LIST_COUNT> &favorite_selection_indices() {
  static std::array<int, FAVORITE_LIST_COUNT> indices{};
  return indices;
}

inline int &favorite_selected_index_ref(int list_index) {
  auto &indices = favorite_selection_indices();
  return indices[list_index];
}

inline int menu_slot_count() {
  return FAVORITE_LIST_COUNT_INT + (notifications_mode_enabled() ? 1 : 0) + 1;
}

inline int notifications_menu_index() {
  return FAVORITE_LIST_COUNT_INT;
}

inline int info_menu_index() {
  return FAVORITE_LIST_COUNT_INT + (notifications_mode_enabled() ? 1 : 0);
}

inline bool menu_index_is_favorite(int menu_index) {
  return menu_index >= 0 && menu_index < FAVORITE_LIST_COUNT_INT;
}

inline bool menu_index_is_notifications(int menu_index) {
  return notifications_mode_enabled() && menu_index == notifications_menu_index();
}

inline bool menu_index_is_info(int menu_index) {
  return menu_index == info_menu_index();
}

inline int first_available_menu_index() {
  for (int i = 0; i < FAVORITE_LIST_COUNT_INT; i++) {
    if (favorite_list_is_available(i)) {
      return i;
    }
  }
  if (notifications_mode_enabled()) {
    return notifications_menu_index();
  }
  return info_menu_index();
}

inline int clamp_menu_index(int menu_index) {
  int count = menu_slot_count();
  if (count <= 0) {
    return 0;
  }
  if (menu_index < 0 || menu_index >= count) {
    return first_available_menu_index();
  }
  if (menu_index_is_favorite(menu_index) && !favorite_list_is_available(menu_index)) {
    return first_available_menu_index();
  }
  if (!menu_index_is_favorite(menu_index) && !menu_index_is_notifications(menu_index) && !menu_index_is_info(menu_index)) {
    return first_available_menu_index();
  }
  return menu_index;
}

inline int next_available_menu_index(int current, int step) {
  int count = menu_slot_count();
  if (count <= 0) {
    return 0;
  }
  current = clamp_menu_index(current);
  if (step == 0) {
    return current;
  }

  for (int i = 1; i <= count; i++) {
    int next = (current + (step > 0 ? i : -i) + count * 2) % count;
    if (menu_index_is_favorite(next) && !favorite_list_is_available(next)) {
      continue;
    }
    return next;
  }
  return first_available_menu_index();
}

inline const char *menu_index_title(int menu_index) {
  if (menu_index_is_favorite(menu_index)) {
    return favorite_list_title(menu_index);
  }
  if (menu_index_is_notifications(menu_index)) {
    return mode_title(REMOTE_MODE_NOTIFICATIONS);
  }
  return mode_title(REMOTE_MODE_INFO);
}

struct UiMenuHeader {
  const char *title;
  const char *icon;
};

inline UiMenuHeader ui_menu_header(int menu_index, RemoteMode current_mode) {
  UiMenuHeader header{};
  header.title = menu_index_title(menu_index);
  header.icon = menu_index_is_favorite(menu_index)
      ? mode_icon(current_mode)
      : mode_icon(menu_index_is_notifications(menu_index) ? REMOTE_MODE_NOTIFICATIONS : REMOTE_MODE_INFO);
  return header;
}

inline int mode_item_count(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return LIGHT_LIST_COUNT;
    case REMOTE_MODE_SWITCHES:
      return SWITCH_LIST_COUNT;
    case REMOTE_MODE_CLIMATE:
      return CLIMATE_LIST_COUNT;
    case REMOTE_MODE_WATER_HEATERS:
      return WATER_HEATER_LIST_COUNT;
    case REMOTE_MODE_HUMIDIFIERS:
      return HUMIDIFIER_LIST_COUNT;
    case REMOTE_MODE_FANS:
      return FAN_LIST_COUNT;
    case REMOTE_MODE_COVERS:
      return COVER_LIST_COUNT;
    case REMOTE_MODE_LOCKS:
      return LOCK_LIST_COUNT;
    case REMOTE_MODE_MEDIA:
      return MEDIA_PLAYER_LIST_COUNT;
    case REMOTE_MODE_SENSORS:
      return SENSOR_LIST_COUNT;
    case REMOTE_MODE_AUTOMATION:
      return AUTOMATION_LIST_COUNT;
    case REMOTE_MODE_ALARMS:
      return ALARM_LIST_COUNT;
    case REMOTE_MODE_NOTIFICATIONS:
      return notifications_mode_enabled() ? notification_mode_item_count() : 0;
    case REMOTE_MODE_WEATHER:
      return WEATHER_LIST_COUNT;
    case REMOTE_MODE_INFO:
      return INFO_ITEM_COUNT;
    default:
      return 0;
  }
}

inline bool mode_is_available(RemoteMode mode) {
  return mode_item_count(mode) > 0;
}

inline RemoteMode first_available_mode() {
  for (RemoteMode mode : MENU_MODE_ORDER) {
    if (mode_is_available(mode)) {
      return mode;
    }
  }
  return REMOTE_MODE_INFO;
}

inline RemoteMode next_available_mode(RemoteMode current, int step) {
  if (step == 0) {
    return mode_is_available(current) ? current : first_available_mode();
  }

  int current_pos = 0;
  for (int i = 0; i < REMOTE_MODE_COUNT; i++) {
    if (MENU_MODE_ORDER[i] == current) {
      current_pos = i;
      break;
    }
  }

  for (int i = 1; i <= REMOTE_MODE_COUNT; i++) {
    int next_pos = (current_pos + (step > 0 ? i : -i) + REMOTE_MODE_COUNT * 2) % REMOTE_MODE_COUNT;
    RemoteMode mode = MENU_MODE_ORDER[next_pos];
    if (mode_is_available(mode)) {
      return mode;
    }
  }
  return first_available_mode();
}

inline const char *mode_item_name_cstr(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return indexed_entity_name_cstr(LIGHT_LIST, LIGHT_LIST_COUNT, idx);
    case REMOTE_MODE_SWITCHES:
      return indexed_entity_name_cstr(SWITCH_LIST, SWITCH_LIST_COUNT, idx);
    case REMOTE_MODE_CLIMATE:
      return indexed_entity_name_cstr(CLIMATE_LIST, CLIMATE_LIST_COUNT, idx);
    case REMOTE_MODE_WATER_HEATERS:
      return indexed_entity_name_cstr(WATER_HEATER_LIST, WATER_HEATER_LIST_COUNT, idx);
    case REMOTE_MODE_HUMIDIFIERS:
      return indexed_entity_name_cstr(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, idx);
    case REMOTE_MODE_FANS:
      return indexed_entity_name_cstr(FAN_LIST, FAN_LIST_COUNT, idx);
    case REMOTE_MODE_COVERS:
      return indexed_entity_name_cstr(COVER_LIST, COVER_LIST_COUNT, idx);
    case REMOTE_MODE_LOCKS:
      return indexed_entity_name_cstr(LOCK_LIST, LOCK_LIST_COUNT, idx);
    case REMOTE_MODE_MEDIA:
      return indexed_entity_name_cstr(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, idx);
    case REMOTE_MODE_SENSORS:
      return indexed_entity_name_cstr(SENSOR_LIST, SENSOR_LIST_COUNT, idx);
    case REMOTE_MODE_AUTOMATION:
      return indexed_entity_name_cstr(AUTOMATION_LIST, AUTOMATION_LIST_COUNT, idx);
    case REMOTE_MODE_ALARMS:
      return indexed_entity_name_cstr(ALARM_LIST, ALARM_LIST_COUNT, idx);
    case REMOTE_MODE_WEATHER:
      return indexed_entity_name_cstr(WEATHER_LIST, WEATHER_LIST_COUNT, idx);
    case REMOTE_MODE_INFO:
      return indexed_value_cstr(INFO_ITEM_NAMES, INFO_ITEM_COUNT, idx);
    case REMOTE_MODE_NOTIFICATIONS:
    default:
      return nullptr;
  }
}

inline const char *mode_item_entity_cstr(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return indexed_entity_id_cstr(LIGHT_LIST, LIGHT_LIST_COUNT, idx);
    case REMOTE_MODE_SWITCHES:
      return indexed_entity_id_cstr(SWITCH_LIST, SWITCH_LIST_COUNT, idx);
    case REMOTE_MODE_CLIMATE:
      return indexed_entity_id_cstr(CLIMATE_LIST, CLIMATE_LIST_COUNT, idx);
    case REMOTE_MODE_WATER_HEATERS:
      return indexed_entity_id_cstr(WATER_HEATER_LIST, WATER_HEATER_LIST_COUNT, idx);
    case REMOTE_MODE_HUMIDIFIERS:
      return indexed_entity_id_cstr(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, idx);
    case REMOTE_MODE_FANS:
      return indexed_entity_id_cstr(FAN_LIST, FAN_LIST_COUNT, idx);
    case REMOTE_MODE_COVERS:
      return indexed_entity_id_cstr(COVER_LIST, COVER_LIST_COUNT, idx);
    case REMOTE_MODE_LOCKS:
      return indexed_entity_id_cstr(LOCK_LIST, LOCK_LIST_COUNT, idx);
    case REMOTE_MODE_MEDIA:
      return indexed_entity_id_cstr(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, idx);
    case REMOTE_MODE_SENSORS:
      return indexed_entity_id_cstr(SENSOR_LIST, SENSOR_LIST_COUNT, idx);
    case REMOTE_MODE_AUTOMATION:
      return indexed_entity_id_cstr(AUTOMATION_LIST, AUTOMATION_LIST_COUNT, idx);
    case REMOTE_MODE_ALARMS:
      return indexed_entity_id_cstr(ALARM_LIST, ALARM_LIST_COUNT, idx);
    case REMOTE_MODE_WEATHER:
      return indexed_entity_id_cstr(WEATHER_LIST, WEATHER_LIST_COUNT, idx);
    case REMOTE_MODE_INFO:
      return indexed_value_cstr(INFO_ITEM_ENTITIES, INFO_ITEM_COUNT, idx);
    case REMOTE_MODE_NOTIFICATIONS:
    default:
      return nullptr;
  }
}

inline std::string mode_item_name(RemoteMode mode, int idx) {
  if (mode == REMOTE_MODE_NOTIFICATIONS) {
    return notification_mode_item_name(idx);
  }
  const char *name = mode_item_name_cstr(mode, idx);
  return name != nullptr ? std::string(name) : std::string();
}

inline std::string mode_item_entity(RemoteMode mode, int idx) {
  if (mode == REMOTE_MODE_NOTIFICATIONS) {
    return notification_mode_item_entity(idx);
  }
  const char *entity = mode_item_entity_cstr(mode, idx);
  return entity != nullptr ? std::string(entity) : std::string();
}

enum AutomationKind {
  AUTOMATION_KIND_AUTOMATION = 0,
  AUTOMATION_KIND_SCRIPT = 1,
  AUTOMATION_KIND_SCENE = 2,
};

enum AlarmArmMode {
  ALARM_ARM_MODE_AWAY = 0,
  ALARM_ARM_MODE_HOME = 1,
  ALARM_ARM_MODE_NIGHT = 2,
  ALARM_ARM_MODE_VACATION = 3,
};

inline constexpr int ALARM_ARM_MODE_COUNT = 4;

inline AlarmArmMode clamp_alarm_arm_mode(int value) {
  if (value < 0 || value >= ALARM_ARM_MODE_COUNT) {
    return ALARM_ARM_MODE_AWAY;
  }
  return static_cast<AlarmArmMode>(value);
}

inline AlarmArmMode next_alarm_arm_mode(AlarmArmMode mode, int step = 1) {
  int next = (static_cast<int>(mode) + (step % ALARM_ARM_MODE_COUNT) + ALARM_ARM_MODE_COUNT) % ALARM_ARM_MODE_COUNT;
  return static_cast<AlarmArmMode>(next);
}

inline const char *alarm_arm_mode_selection_label(AlarmArmMode mode) {
  switch (mode) {
    case ALARM_ARM_MODE_HOME:
      return "ARM HOME";
    case ALARM_ARM_MODE_NIGHT:
      return "ARM NIGHT";
    case ALARM_ARM_MODE_VACATION:
      return "ARM VACATION";
    case ALARM_ARM_MODE_AWAY:
    default:
      return "ARM AWAY";
  }
}

inline const char *alarm_arm_mode_hold_label(AlarmArmMode mode) {
  switch (mode) {
    case ALARM_ARM_MODE_HOME:
      return "HOLD TO ARM HOME";
    case ALARM_ARM_MODE_NIGHT:
      return "HOLD TO ARM NIGHT";
    case ALARM_ARM_MODE_VACATION:
      return "HOLD TO ARM VACATION";
    case ALARM_ARM_MODE_AWAY:
    default:
      return "HOLD TO ARM AWAY";
  }
}

inline const char *alarm_expected_armed_state(AlarmArmMode mode) {
  switch (mode) {
    case ALARM_ARM_MODE_HOME: return "armed_home";
    case ALARM_ARM_MODE_NIGHT: return "armed_night";
    case ALARM_ARM_MODE_VACATION: return "armed_vacation";
    case ALARM_ARM_MODE_AWAY:
    default: return "armed_away";
  }
}

inline bool alarm_action_is_arm(const std::string &state, AlarmArmMode arm_mode) {
  if (state == "unknown") return false;
  return state != alarm_expected_armed_state(arm_mode);
}

struct ModeSelectionStateRefs {
  int &light_idx;
  int &switch_idx;
  int &climate_idx;
  int &water_heater_idx;
  int &lock_idx;
  int &cover_idx;
  int &media_idx;
  int &automation_idx;
  int &weather_idx;
  int &fan_idx;
  int &humidifier_idx;
  int &sensor_idx;
  int &alarm_idx;
  int &notification_idx;
  int &info_idx;
};

struct CurrentModeSelectionContext {
  RemoteMode mode;
  int count;
  int index;
  int *selected_index;
};

struct CurrentUiSelectionContext {
  RemoteMode mode;
  int count;
  int index;
  int resolved_index;
  int *selected_index;
  int menu_index;
  int favorite_list_index;
  bool is_favorite;
};

inline ModeSelectionStateRefs make_mode_selection_state_refs(
    int &light_idx, int &switch_idx, int &climate_idx, int &water_heater_idx, int &lock_idx, int &cover_idx, int &media_idx,
    int &automation_idx, int &weather_idx, int &fan_idx, int &humidifier_idx, int &sensor_idx, int &alarm_idx,
    int &notification_idx, int &info_idx) {
  return {light_idx, switch_idx, climate_idx, water_heater_idx, lock_idx, cover_idx, media_idx, automation_idx,
          weather_idx, fan_idx, humidifier_idx, sensor_idx, alarm_idx, notification_idx, info_idx};
}

inline AutomationKind automation_kind(int idx) {
  if (idx < 0 || idx >= AUTOMATION_LIST_COUNT) {
    return AUTOMATION_KIND_SCRIPT;
  }

  std::string entity_id = AUTOMATION_LIST[idx].entity_id;
  size_t separator = entity_id.find('.');
  std::string domain = separator == std::string::npos ? entity_id : entity_id.substr(0, separator);

  if (domain == "automation") {
    return AUTOMATION_KIND_AUTOMATION;
  }
  if (domain == "scene") {
    return AUTOMATION_KIND_SCENE;
  }
  return AUTOMATION_KIND_SCRIPT;
}

inline bool automation_supports_enabled_state(int idx) {
  return automation_kind(idx) == AUTOMATION_KIND_AUTOMATION;
}

inline const char *automation_kind_label(int idx) {
  switch (automation_kind(idx)) {
    case AUTOMATION_KIND_AUTOMATION:
      return "AUTOMATION";
    case AUTOMATION_KIND_SCENE:
      return "SCENE";
    case AUTOMATION_KIND_SCRIPT:
    default:
      return "SCRIPT";
  }
}

inline int &selected_mode_index_ref(RemoteMode mode, ModeSelectionStateRefs refs) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return refs.light_idx;
    case REMOTE_MODE_SWITCHES:
      return refs.switch_idx;
    case REMOTE_MODE_CLIMATE:
      return refs.climate_idx;
    case REMOTE_MODE_WATER_HEATERS:
      return refs.water_heater_idx;
    case REMOTE_MODE_HUMIDIFIERS:
      return refs.humidifier_idx;
    case REMOTE_MODE_FANS:
      return refs.fan_idx;
    case REMOTE_MODE_COVERS:
      return refs.cover_idx;
    case REMOTE_MODE_LOCKS:
      return refs.lock_idx;
    case REMOTE_MODE_MEDIA:
      return refs.media_idx;
    case REMOTE_MODE_SENSORS:
      return refs.sensor_idx;
    case REMOTE_MODE_AUTOMATION:
      return refs.automation_idx;
    case REMOTE_MODE_ALARMS:
      return refs.alarm_idx;
    case REMOTE_MODE_NOTIFICATIONS:
      return refs.notification_idx;
    case REMOTE_MODE_WEATHER:
      return refs.weather_idx;
    case REMOTE_MODE_INFO:
    default:
      return refs.info_idx;
  }
}

inline int clamp_mode_index(int idx, int count) {
  if (count <= 0) {
    return 0;
  }
  return (idx >= 0 && idx < count) ? idx : 0;
}

inline CurrentModeSelectionContext resolve_current_mode_selection_context(
    int &current_mode_value, ModeSelectionStateRefs refs, bool ensure_available = true) {
  RemoteMode mode = static_cast<RemoteMode>(current_mode_value);
  if (ensure_available && !mode_is_available(mode)) {
    mode = first_available_mode();
    current_mode_value = mode;
  }

  int count = mode_item_count(mode);
  int &selected_index = selected_mode_index_ref(mode, refs);
  selected_index = clamp_mode_index(selected_index, count);

  return {mode, count, selected_index, &selected_index};
}

inline CurrentUiSelectionContext resolve_current_ui_selection_context(
    int &current_mode_value, int &current_menu_index, ModeSelectionStateRefs refs) {
  current_menu_index = clamp_menu_index(current_menu_index);

  if (menu_index_is_favorite(current_menu_index)) {
    int list_index = current_menu_index;
    int count = favorite_list_item_count(list_index);
    int &favorite_index = favorite_selected_index_ref(list_index);
    favorite_index = clamp_mode_index(favorite_index, count);

    const FavoriteEntity *entry = favorite_list_entry(list_index, favorite_index);
    if (entry == nullptr) {
      current_mode_value = REMOTE_MODE_INFO;
      return {REMOTE_MODE_INFO, 0, 0, 0, &favorite_index, current_menu_index, list_index, true};
    }

    RemoteMode mode = favorite_entity_mode(entry->entity_id);
    int resolved_index = favorite_entity_mode_index(mode, entry->entity_id);
    if (resolved_index < 0) {
      current_mode_value = REMOTE_MODE_INFO;
      return {REMOTE_MODE_INFO, 0, favorite_index, 0, &favorite_index, current_menu_index, list_index, true};
    }

    int &mode_index = selected_mode_index_ref(mode, refs);
    mode_index = resolved_index;
    current_mode_value = mode;
    return {mode, count, favorite_index, resolved_index, &favorite_index, current_menu_index, list_index, true};
  }

  RemoteMode mode = menu_index_is_notifications(current_menu_index) ? REMOTE_MODE_NOTIFICATIONS : REMOTE_MODE_INFO;
  current_mode_value = mode;
  int count = mode_item_count(mode);
  int &selected_index = selected_mode_index_ref(mode, refs);
  selected_index = clamp_mode_index(selected_index, count);
  return {mode, count, selected_index, selected_index, &selected_index, current_menu_index, -1, false};
}

inline const char *ui_selection_item_name_cstr(const CurrentUiSelectionContext &selection) {
  if (selection.is_favorite) {
    const FavoriteEntity *entry = favorite_list_entry(selection.favorite_list_index, selection.index);
    return entry != nullptr ? entry->name : "";
  }
  return mode_item_name_cstr(selection.mode, selection.resolved_index);
}

inline const char *ui_selection_item_entity_cstr(const CurrentUiSelectionContext &selection) {
  if (selection.is_favorite) {
    const FavoriteEntity *entry = favorite_list_entry(selection.favorite_list_index, selection.index);
    return entry != nullptr ? entry->entity_id : "";
  }
  return mode_item_entity_cstr(selection.mode, selection.resolved_index);
}

inline std::string ui_selection_item_name(const CurrentUiSelectionContext &selection) {
  if (selection.is_favorite) {
    const FavoriteEntity *entry = favorite_list_entry(selection.favorite_list_index, selection.index);
    return entry != nullptr ? entry->name : "";
  }
  return mode_item_name(selection.mode, selection.resolved_index);
}

inline std::string ui_selection_item_entity(const CurrentUiSelectionContext &selection) {
  if (selection.is_favorite) {
    const FavoriteEntity *entry = favorite_list_entry(selection.favorite_list_index, selection.index);
    return entry != nullptr ? entry->entity_id : "";
  }
  return mode_item_entity(selection.mode, selection.resolved_index);
}

inline int wrapped_mode_index(int idx, int count, int step) {
  if (count <= 0) {
    return 0;
  }
  idx = clamp_mode_index(idx, count);
  idx = (idx + (step % count) + count) % count;
  return idx;
}

inline const std::string &unknown_string() {
  static const std::string unknown = "unknown";
  return unknown;
}

inline const std::string &empty_string() {
  static const std::string empty;
  return empty;
}

template <typename T, size_t Count>
inline std::array<T, Count> filled_array(const T &value) {
  std::array<T, Count> result{};
  result.fill(value);
  return result;
}
