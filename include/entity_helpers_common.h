#pragma once

#include <array>
#include <cmath>
#include <cstdlib>
#include <string>

#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/json/json_util.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/string_ref.h"

enum RemoteMode {
  REMOTE_MODE_LIGHTS = 0,
  REMOTE_MODE_SWITCHES = 1,
  REMOTE_MODE_CLIMATE = 2,
  REMOTE_MODE_HUMIDIFIERS = 3,
  REMOTE_MODE_FANS = 4,
  REMOTE_MODE_COVERS = 5,
  REMOTE_MODE_LOCKS = 6,
  REMOTE_MODE_MEDIA = 7,
  REMOTE_MODE_SENSORS = 8,
  REMOTE_MODE_AUTOMATION = 9,
  REMOTE_MODE_NOTIFICATIONS = 10,
  REMOTE_MODE_WEATHER = 11,
  REMOTE_MODE_INFO = 12,
  REMOTE_MODE_ALARMS = 13,
};

static constexpr int REMOTE_MODE_COUNT = 14;
static constexpr RemoteMode MENU_MODE_ORDER[] = {
    REMOTE_MODE_LIGHTS,
    REMOTE_MODE_SWITCHES,
    REMOTE_MODE_CLIMATE,
    REMOTE_MODE_HUMIDIFIERS,
    REMOTE_MODE_FANS,
    REMOTE_MODE_COVERS,
    REMOTE_MODE_LOCKS,
    REMOTE_MODE_MEDIA,
    REMOTE_MODE_SENSORS,
    REMOTE_MODE_AUTOMATION,
    REMOTE_MODE_ALARMS,
    REMOTE_MODE_NOTIFICATIONS,
    REMOTE_MODE_WEATHER,
    REMOTE_MODE_INFO,
};

struct LightEntity {
  const char *name;
  const char *entity_id;
};

struct SwitchEntity {
  const char *name;
  const char *entity_id;
};

struct ClimateEntity {
  const char *name;
  const char *entity_id;
};

struct HumidifierEntity {
  const char *name;
  const char *entity_id;
};

struct FanEntity {
  const char *name;
  const char *entity_id;
};

struct LockEntity {
  const char *name;
  const char *entity_id;
};

struct CoverEntity {
  const char *name;
  const char *entity_id;
};

struct MediaEntity {
  const char *name;
  const char *entity_id;
  const char *sources = nullptr;
};

struct SensorEntity {
  const char *name;
  const char *entity_id;
};

struct AutomationEntity {
  const char *name;
  const char *entity_id;
};

struct AlarmEntity {
  const char *name;
  const char *entity_id;
};

struct WeatherEntity {
  const char *name;
  const char *entity_id;
};

#include "local_entities.h"

static constexpr int LIGHT_LIST_COUNT = sizeof(LIGHT_LIST) / sizeof(LIGHT_LIST[0]);
static constexpr int SWITCH_LIST_COUNT = sizeof(SWITCH_LIST) / sizeof(SWITCH_LIST[0]);
static constexpr int CLIMATE_LIST_COUNT = sizeof(CLIMATE_LIST) / sizeof(CLIMATE_LIST[0]);
static constexpr int HUMIDIFIER_LIST_COUNT = sizeof(HUMIDIFIER_LIST) / sizeof(HUMIDIFIER_LIST[0]);
static constexpr int FAN_LIST_COUNT = sizeof(FAN_LIST) / sizeof(FAN_LIST[0]);
static constexpr int COVER_LIST_COUNT = sizeof(COVER_LIST) / sizeof(COVER_LIST[0]);
static constexpr int LOCK_LIST_COUNT = sizeof(LOCK_LIST) / sizeof(LOCK_LIST[0]);
static constexpr int MEDIA_PLAYER_LIST_COUNT = sizeof(MEDIA_PLAYER_LIST) / sizeof(MEDIA_PLAYER_LIST[0]);
static constexpr int SENSOR_LIST_COUNT = sizeof(SENSOR_LIST) / sizeof(SENSOR_LIST[0]);
static constexpr int AUTOMATION_LIST_COUNT = sizeof(AUTOMATION_LIST) / sizeof(AUTOMATION_LIST[0]);
static constexpr int ALARM_LIST_COUNT = sizeof(ALARM_LIST) / sizeof(ALARM_LIST[0]);
static constexpr int WEATHER_LIST_COUNT = sizeof(WEATHER_LIST) / sizeof(WEATHER_LIST[0]);
static const char *const INFO_ITEM_NAMES[] = {"Time & Date", "Network", "Version"};
static const char *const INFO_ITEM_ENTITIES[] = {"info.date", "info.network", "info.version"};
static const int INFO_ITEM_COUNT = sizeof(INFO_ITEM_NAMES) / sizeof(INFO_ITEM_NAMES[0]);
static constexpr int NOTIFICATION_FEED_MAX_ITEMS = 16;

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

static inline int notification_mode_item_count();
static inline std::string notification_mode_item_name(int idx);
static inline std::string notification_mode_item_entity(int idx);
static inline const std::string &notification_id_for_index(int idx);
static inline const std::string &media_source_list_for_index(int idx);
static inline const std::string &media_device_class_for_index(int idx);

static inline bool ha_state_missing(const std::string &value) {
  return value.empty() || value == "unknown" || value == "unavailable" || value == "None";
}

static inline std::string ha_state_or_unknown(esphome::StringRef state) {
  std::string value = state.str();
  return ha_state_missing(value) ? "unknown" : value;
}

static inline float ha_parse_float(esphome::StringRef state) {
  std::string value = state.str();
  return ha_state_missing(value) ? NAN : strtof(value.c_str(), nullptr);
}

static inline std::string trim_copy(const std::string &value) {
  size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

static inline std::string remote_state_label(const std::string &raw, const char *fallback = "SYNCING") {
  std::string label = raw;
  for (auto &ch : label) {
    if (ch >= 'a' && ch <= 'z') {
      ch = ch - 'a' + 'A';
    } else if (ch == '_') {
      ch = ' ';
    }
  }
  if (label.empty() || label == "UNKNOWN") {
    return std::string(fallback);
  }
  return label;
}

static inline int clamp_percent_value(float value, float scale = 1.0f, int min_value = 0) {
  int pct = (int) roundf(value * scale);
  if (pct < min_value) pct = min_value;
  if (pct > 100) pct = 100;
  return pct;
}

static inline bool lock_state_matches_expected(const std::string &state, const std::string &expected_state) {
  return state == expected_state;
}

static inline std::string lock_operation_feedback_for_state(const std::string &state) {
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

static inline bool cover_state_matches_expected(const std::string &state, float position, const std::string &expected_state) {
  const bool reached_open = state == "open" || (!std::isnan(position) && position >= 99.0f);
  const bool reached_closed = state == "closed" || (!std::isnan(position) && position <= 1.0f);
  return (expected_state == "open" && reached_open) || (expected_state == "closed" && reached_closed);
}

static inline std::string cover_operation_feedback_for_state(const std::string &state, float position) {
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

static inline bool alarm_state_matches_expected(const std::string &state, const std::string &expected_state) {
  return state == expected_state;
}

static inline std::string alarm_operation_feedback_for_state(const std::string &state) {
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

static inline int delimited_option_count(const std::string &source_list) {
  int count = 0;
  size_t start = 0;
  while (start <= source_list.size()) {
    size_t end = source_list.find('|', start);
    std::string item = trim_copy(source_list.substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (!item.empty()) {
      count++;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return count;
}

static inline std::string delimited_option_at(const std::string &source_list, int option_idx) {
  if (option_idx < 0) {
    return "";
  }
  int current_idx = 0;
  size_t start = 0;
  while (start <= source_list.size()) {
    size_t end = source_list.find('|', start);
    std::string item = trim_copy(source_list.substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (!item.empty()) {
      if (current_idx == option_idx) {
        return item;
      }
      current_idx++;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return "";
}

static inline bool notifications_mode_enabled() {
  return NOTIFICATION_FEED_ENTITY[0] != '\0';
}

static inline std::string media_configured_source_list_for_index(int idx) {
  if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
    return "";
  }
  const char *sources = MEDIA_PLAYER_LIST[idx].sources;
  return (sources == nullptr) ? "" : std::string(sources);
}

static inline std::string active_media_source_list_for_index(int idx) {
  std::string tracked_sources = media_source_list_for_index(idx);
  std::string device_class = media_device_class_for_index(idx);
  if ((device_class == "tv" || device_class == "receiver") && !tracked_sources.empty()) {
    return tracked_sources;
  }
  return media_configured_source_list_for_index(idx);
}

static inline int media_source_option_count(int idx) {
  return delimited_option_count(active_media_source_list_for_index(idx));
}

static inline std::string media_source_option_at(int idx, int option_idx) {
  return delimited_option_at(active_media_source_list_for_index(idx), option_idx);
}

static inline std::string next_media_source_for_index(int idx, const std::string &current_source) {
  int count = media_source_option_count(idx);
  if (count <= 0) {
    return "";
  }

  std::string current = trim_copy(current_source);
  for (int i = 0; i < count; i++) {
    if (media_source_option_at(idx, i) == current) {
      return media_source_option_at(idx, (i + 1) % count);
    }
  }
  return media_source_option_at(idx, 0);
}

template <typename Entity>
static inline int find_entity_index(const Entity *entities, int count, const std::string &entity_id) {
  for (int i = 0; i < count; i++) {
    if (entity_id == entities[i].entity_id) {
      return i;
    }
  }
  return -1;
}

template <typename RequestFn>
static inline void request_all_states_by_count(int count, RequestFn request_fn) {
  for (int i = 0; i < count; i++) {
    request_fn(i);
  }
}

template <typename Entity>
static inline std::string indexed_entity_name(const Entity *entities, int count, int idx) {
  return (idx >= 0 && idx < count) ? entities[idx].name : "";
}

template <typename Entity>
static inline std::string indexed_entity_id(const Entity *entities, int count, int idx) {
  return (idx >= 0 && idx < count) ? entities[idx].entity_id : "";
}

static inline std::string indexed_value(const char *const *values, int count, int idx) {
  return (idx >= 0 && idx < count) ? values[idx] : "";
}

static inline const char *mode_title(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return "LIGHTS";
    case REMOTE_MODE_SWITCHES:
      return "SWITCHES";
    case REMOTE_MODE_CLIMATE:
      return "CLIMATE";
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

static inline const char *mode_icon(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return "\ue90f";
    case REMOTE_MODE_SWITCHES:
      return "\ue1f4";
    case REMOTE_MODE_CLIMATE:
      return "\uf076";
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

static inline int mode_item_count(RemoteMode mode) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return LIGHT_LIST_COUNT;
    case REMOTE_MODE_SWITCHES:
      return SWITCH_LIST_COUNT;
    case REMOTE_MODE_CLIMATE:
      return CLIMATE_LIST_COUNT;
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

static inline bool mode_is_available(RemoteMode mode) {
  return mode_item_count(mode) > 0;
}

static inline RemoteMode first_available_mode() {
  for (RemoteMode mode : MENU_MODE_ORDER) {
    if (mode_is_available(mode)) {
      return mode;
    }
  }
  return REMOTE_MODE_INFO;
}

static inline RemoteMode next_available_mode(RemoteMode current, int step) {
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

static inline std::string mode_item_name(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return indexed_entity_name(LIGHT_LIST, LIGHT_LIST_COUNT, idx);
    case REMOTE_MODE_SWITCHES:
      return indexed_entity_name(SWITCH_LIST, SWITCH_LIST_COUNT, idx);
    case REMOTE_MODE_CLIMATE:
      return indexed_entity_name(CLIMATE_LIST, CLIMATE_LIST_COUNT, idx);
    case REMOTE_MODE_HUMIDIFIERS:
      return indexed_entity_name(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, idx);
    case REMOTE_MODE_FANS:
      return indexed_entity_name(FAN_LIST, FAN_LIST_COUNT, idx);
    case REMOTE_MODE_COVERS:
      return indexed_entity_name(COVER_LIST, COVER_LIST_COUNT, idx);
    case REMOTE_MODE_LOCKS:
      return indexed_entity_name(LOCK_LIST, LOCK_LIST_COUNT, idx);
    case REMOTE_MODE_MEDIA:
      return indexed_entity_name(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, idx);
    case REMOTE_MODE_SENSORS:
      return indexed_entity_name(SENSOR_LIST, SENSOR_LIST_COUNT, idx);
    case REMOTE_MODE_AUTOMATION:
      return indexed_entity_name(AUTOMATION_LIST, AUTOMATION_LIST_COUNT, idx);
    case REMOTE_MODE_ALARMS:
      return indexed_entity_name(ALARM_LIST, ALARM_LIST_COUNT, idx);
    case REMOTE_MODE_NOTIFICATIONS:
      return notification_mode_item_name(idx);
    case REMOTE_MODE_WEATHER:
      return indexed_entity_name(WEATHER_LIST, WEATHER_LIST_COUNT, idx);
    case REMOTE_MODE_INFO:
      return indexed_value(INFO_ITEM_NAMES, INFO_ITEM_COUNT, idx);
    default:
      return "";
  }
}

static inline std::string mode_item_entity(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return indexed_entity_id(LIGHT_LIST, LIGHT_LIST_COUNT, idx);
    case REMOTE_MODE_SWITCHES:
      return indexed_entity_id(SWITCH_LIST, SWITCH_LIST_COUNT, idx);
    case REMOTE_MODE_CLIMATE:
      return indexed_entity_id(CLIMATE_LIST, CLIMATE_LIST_COUNT, idx);
    case REMOTE_MODE_HUMIDIFIERS:
      return indexed_entity_id(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, idx);
    case REMOTE_MODE_FANS:
      return indexed_entity_id(FAN_LIST, FAN_LIST_COUNT, idx);
    case REMOTE_MODE_COVERS:
      return indexed_entity_id(COVER_LIST, COVER_LIST_COUNT, idx);
    case REMOTE_MODE_LOCKS:
      return indexed_entity_id(LOCK_LIST, LOCK_LIST_COUNT, idx);
    case REMOTE_MODE_MEDIA:
      return indexed_entity_id(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, idx);
    case REMOTE_MODE_SENSORS:
      return indexed_entity_id(SENSOR_LIST, SENSOR_LIST_COUNT, idx);
    case REMOTE_MODE_AUTOMATION:
      return indexed_entity_id(AUTOMATION_LIST, AUTOMATION_LIST_COUNT, idx);
    case REMOTE_MODE_ALARMS:
      return indexed_entity_id(ALARM_LIST, ALARM_LIST_COUNT, idx);
    case REMOTE_MODE_NOTIFICATIONS:
      return notification_mode_item_entity(idx);
    case REMOTE_MODE_WEATHER:
      return indexed_entity_id(WEATHER_LIST, WEATHER_LIST_COUNT, idx);
    case REMOTE_MODE_INFO:
      return indexed_value(INFO_ITEM_ENTITIES, INFO_ITEM_COUNT, idx);
    default:
      return "";
  }
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

static constexpr int ALARM_ARM_MODE_COUNT = 4;

static inline AlarmArmMode clamp_alarm_arm_mode(int value) {
  if (value < 0 || value >= ALARM_ARM_MODE_COUNT) {
    return ALARM_ARM_MODE_AWAY;
  }
  return static_cast<AlarmArmMode>(value);
}

static inline AlarmArmMode next_alarm_arm_mode(AlarmArmMode mode, int step = 1) {
  int next = (static_cast<int>(mode) + (step % ALARM_ARM_MODE_COUNT) + ALARM_ARM_MODE_COUNT) % ALARM_ARM_MODE_COUNT;
  return static_cast<AlarmArmMode>(next);
}

static inline const char *alarm_arm_mode_selection_label(AlarmArmMode mode) {
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

static inline const char *alarm_arm_mode_hold_label(AlarmArmMode mode) {
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

static inline AutomationKind automation_kind(int idx) {
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

static inline bool automation_supports_enabled_state(int idx) {
  return automation_kind(idx) == AUTOMATION_KIND_AUTOMATION;
}

static inline const char *automation_kind_label(int idx) {
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

static inline int &selected_mode_index_ref(
    RemoteMode mode, int &light_idx, int &switch_idx, int &climate_idx, int &lock_idx, int &cover_idx, int &media_idx,
    int &automation_idx, int &weather_idx, int &misc_idx, int &fan_idx, int &humidifier_idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      return light_idx;
    case REMOTE_MODE_SWITCHES:
      return switch_idx;
    case REMOTE_MODE_CLIMATE:
      return climate_idx;
    case REMOTE_MODE_HUMIDIFIERS:
      return humidifier_idx;
    case REMOTE_MODE_FANS:
      return fan_idx;
    case REMOTE_MODE_COVERS:
      return cover_idx;
    case REMOTE_MODE_LOCKS:
      return lock_idx;
    case REMOTE_MODE_MEDIA:
      return media_idx;
    case REMOTE_MODE_SENSORS:
      return misc_idx;
    case REMOTE_MODE_AUTOMATION:
      return automation_idx;
    case REMOTE_MODE_ALARMS:
      return misc_idx;
    case REMOTE_MODE_NOTIFICATIONS:
      return misc_idx;
    case REMOTE_MODE_WEATHER:
      return weather_idx;
    case REMOTE_MODE_INFO:
    default:
      return misc_idx;
  }
}

static inline int clamp_mode_index(int idx, int count) {
  if (count <= 0) {
    return 0;
  }
  return (idx >= 0 && idx < count) ? idx : 0;
}

static inline int wrapped_mode_index(int idx, int count, int step) {
  if (count <= 0) {
    return 0;
  }
  idx = clamp_mode_index(idx, count);
  idx = (idx + (step % count) + count) % count;
  return idx;
}

static inline const std::string &unknown_string() {
  static const std::string unknown = "unknown";
  return unknown;
}

static inline const std::string &empty_string() {
  static const std::string empty;
  return empty;
}

template <typename T, size_t Count>
static inline std::array<T, Count> filled_array(const T &value) {
  std::array<T, Count> result{};
  result.fill(value);
  return result;
}
