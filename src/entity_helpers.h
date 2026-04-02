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

template <typename Entity, int Count>
class SingleStateTracker : public esphome::api::CustomAPIDevice {
 public:
  explicit SingleStateTracker(const Entity *entities) : entities_(entities) {}

  void setup() {
    for (int i = 0; i < Count; i++) {
      this->subscribe_homeassistant_state(&SingleStateTracker::on_state_, this->entities_[i].entity_id);
    }
    this->request_all_states();
  }

  void request_state(int idx) {
    if (idx < 0 || idx >= Count) {
      return;
    }

    const char *entity_id = this->entities_[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
  }

  void request_all_states() { request_all_states_by_count(Count, [this](int i) { this->request_state(i); }); }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= Count) {
      return unknown_string();
    }
    return this->state_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(this->entities_, Count, entity_id);
  }

  const Entity *entities_;
  std::array<std::string, Count> state_{};
};

class LightStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < LIGHT_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&LightStatusTracker::on_light_state_, LIGHT_LIST[i].entity_id);
      this->subscribe_homeassistant_state(
          &LightStatusTracker::on_light_brightness_, LIGHT_LIST[i].entity_id, "brightness");
    }
    this->request_all_states();
  }

  void request_light_state(int idx) {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return;
    }

    const char *entity_id = LIGHT_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "brightness", [this, idx](esphome::StringRef state) { this->store_brightness_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(LIGHT_LIST_COUNT, [this](int i) { this->request_light_state(i); });
  }

  bool has_state(int idx) const { return idx >= 0 && idx < LIGHT_LIST_COUNT && this->has_state_[idx]; }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_brightness(int idx) const { return idx >= 0 && idx < LIGHT_LIST_COUNT && this->has_brightness_[idx]; }

  float brightness(int idx) const {
    if (idx < 0 || idx >= LIGHT_LIST_COUNT) {
      return NAN;
    }
    return this->brightness_[idx];
  }

 protected:
  void on_light_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_light_brightness_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_brightness_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
    this->has_state_[idx] = this->state_[idx] != "unknown";
  }

  void store_brightness_(int idx, esphome::StringRef state) {
    this->brightness_[idx] = ha_parse_float(state);
    this->has_brightness_[idx] = !std::isnan(this->brightness_[idx]);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(LIGHT_LIST, LIGHT_LIST_COUNT, entity_id);
  }

  std::array<std::string, LIGHT_LIST_COUNT> state_{};
  std::array<float, LIGHT_LIST_COUNT> brightness_ = filled_array<float, LIGHT_LIST_COUNT>(NAN);
  std::array<bool, LIGHT_LIST_COUNT> has_state_{};
  std::array<bool, LIGHT_LIST_COUNT> has_brightness_{};
};

using SwitchStatusTracker = SingleStateTracker<SwitchEntity, SWITCH_LIST_COUNT>;

class FanStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < FAN_LIST_COUNT; i++) {
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_state_, FAN_LIST[i].entity_id);
      this->subscribe_homeassistant_state(&FanStatusTracker::on_fan_percentage_, FAN_LIST[i].entity_id, "percentage");
    }
    this->request_all_states();
  }

  void request_fan_state(int idx) {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return;
    }

    const char *entity_id = FAN_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "percentage", [this, idx](esphome::StringRef state) { this->store_percentage_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(FAN_LIST_COUNT, [this](int i) { this->request_fan_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_percentage(int idx) const { return idx >= 0 && idx < FAN_LIST_COUNT && this->has_percentage_[idx]; }

  float percentage(int idx) const {
    if (idx < 0 || idx >= FAN_LIST_COUNT) {
      return NAN;
    }
    return this->percentage_[idx];
  }

 protected:
  void on_fan_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_fan_percentage_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_percentage_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }

  void store_percentage_(int idx, esphome::StringRef state) {
    this->percentage_[idx] = ha_parse_float(state);
    this->has_percentage_[idx] = !std::isnan(this->percentage_[idx]);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(FAN_LIST, FAN_LIST_COUNT, entity_id);
  }

  std::array<std::string, FAN_LIST_COUNT> state_{};
  std::array<float, FAN_LIST_COUNT> percentage_ = filled_array<float, FAN_LIST_COUNT>(NAN);
  std::array<bool, FAN_LIST_COUNT> has_percentage_{};
};

class HumidifierStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < HUMIDIFIER_LIST_COUNT; i++) {
      const char *entity_id = HUMIDIFIER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_mode_, entity_id, "mode");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_available_modes_, entity_id, "available_modes");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_target_humidity_, entity_id, "humidity");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_current_humidity_, entity_id, "current_humidity");
      this->subscribe_homeassistant_state(&HumidifierStatusTracker::on_action_, entity_id, "action");
    }
    this->request_all_states();
  }

  void request_humidifier_state(int idx) {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return;
    }

    const char *entity_id = HUMIDIFIER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "mode", [this, idx](esphome::StringRef state) { this->store_mode_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "available_modes",
        [this, idx](esphome::StringRef state) { this->store_available_modes_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "humidity",
        [this, idx](esphome::StringRef state) { this->store_target_humidity_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "current_humidity",
        [this, idx](esphome::StringRef state) { this->store_current_humidity_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "action", [this, idx](esphome::StringRef state) { this->store_action_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(HUMIDIFIER_LIST_COUNT, [this](int i) { this->request_humidifier_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  const std::string &action(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return unknown_string();
    }
    return this->action_[idx];
  }

  const std::string &mode(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return empty_string();
    }
    return this->mode_[idx];
  }

  const std::string &available_modes(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return empty_string();
    }
    return this->available_modes_[idx];
  }

  float target_humidity(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return NAN;
    }
    return this->target_humidity_[idx];
  }

  float current_humidity(int idx) const {
    if (idx < 0 || idx >= HUMIDIFIER_LIST_COUNT) {
      return NAN;
    }
    return this->current_humidity_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_mode_(idx, state);
    }
  }

  void on_available_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_available_modes_(idx, state);
    }
  }

  void on_target_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_humidity_(idx, state);
    }
  }

  void on_current_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_current_humidity_(idx, state);
    }
  }

  void on_action_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_action_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }
  void store_mode_(int idx, esphome::StringRef state) { this->mode_[idx] = state.str(); }
  void store_available_modes_(int idx, esphome::StringRef state) {
    this->available_modes_[idx].clear();
    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    if (!doc.is<JsonArray>()) {
      return;
    }

    JsonArray mode_list = doc.as<JsonArray>();
    bool first = true;
    for (JsonVariant value : mode_list) {
      std::string mode = trim_copy(value.as<std::string>());
      if (mode.empty()) {
        continue;
      }
      if (!first) {
        this->available_modes_[idx] += "|";
      }
      this->available_modes_[idx] += mode;
      first = false;
    }
  }
  void store_target_humidity_(int idx, esphome::StringRef state) { this->target_humidity_[idx] = ha_parse_float(state); }
  void store_current_humidity_(int idx, esphome::StringRef state) { this->current_humidity_[idx] = ha_parse_float(state); }
  void store_action_(int idx, esphome::StringRef state) { this->action_[idx] = ha_state_or_unknown(state); }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(HUMIDIFIER_LIST, HUMIDIFIER_LIST_COUNT, entity_id);
  }

  std::array<std::string, HUMIDIFIER_LIST_COUNT> state_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> mode_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> available_modes_{};
  std::array<std::string, HUMIDIFIER_LIST_COUNT> action_{};
  std::array<float, HUMIDIFIER_LIST_COUNT> target_humidity_ =
      filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
  std::array<float, HUMIDIFIER_LIST_COUNT> current_humidity_ =
      filled_array<float, HUMIDIFIER_LIST_COUNT>(NAN);
};

class ClimateStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < CLIMATE_LIST_COUNT; i++) {
      const char *entity_id = CLIMATE_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_temperature_, entity_id, "temperature");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_temperature_low_, entity_id, "target_temp_low");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_target_temperature_high_, entity_id, "target_temp_high");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_current_temperature_, entity_id, "current_temperature");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_hvac_action_, entity_id, "hvac_action");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_preset_mode_, entity_id, "preset_mode");
      this->subscribe_homeassistant_state(&ClimateStatusTracker::on_preset_modes_, entity_id, "preset_modes");
    }
    this->request_all_states();
  }

  void request_climate_state(int idx) {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return;
    }

    const char *entity_id = CLIMATE_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "temperature",
        [this, idx](esphome::StringRef state) { this->store_target_temperature_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "target_temp_low",
        [this, idx](esphome::StringRef state) { this->store_target_temperature_low_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "target_temp_high",
        [this, idx](esphome::StringRef state) { this->store_target_temperature_high_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "current_temperature",
        [this, idx](esphome::StringRef state) { this->store_current_temperature_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "hvac_action",
        [this, idx](esphome::StringRef state) { this->store_hvac_action_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "preset_mode", [this, idx](esphome::StringRef state) { this->store_preset_mode_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "preset_modes", [this, idx](esphome::StringRef state) { this->store_preset_modes_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(CLIMATE_LIST_COUNT, [this](int i) { this->request_climate_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  const std::string &hvac_action(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return unknown_string();
    }
    return this->hvac_action_[idx];
  }

  const std::string &preset_mode(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return empty_string();
    }
    return this->preset_mode_[idx];
  }

  bool supports_preset(int idx) const {
    return idx >= 0 && idx < CLIMATE_LIST_COUNT && this->supports_preset_[idx];
  }

  float target_temperature(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_temperature_[idx];
  }

  float current_temperature(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->current_temperature_[idx];
  }

  float target_temperature_low(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_temperature_low_[idx];
  }

  float target_temperature_high(int idx) const {
    if (idx < 0 || idx >= CLIMATE_LIST_COUNT) {
      return NAN;
    }
    return this->target_temperature_high_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_target_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_temperature_(idx, state);
    }
  }

  void on_current_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_current_temperature_(idx, state);
    }
  }

  void on_target_temperature_low_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_temperature_low_(idx, state);
    }
  }

  void on_target_temperature_high_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_target_temperature_high_(idx, state);
    }
  }

  void on_hvac_action_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_hvac_action_(idx, state);
    }
  }

  void on_preset_mode_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_preset_mode_(idx, state);
    }
  }

  void on_preset_modes_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_preset_modes_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_target_temperature_(int idx, esphome::StringRef state) {
    this->target_temperature_[idx] = ha_parse_float(state);
  }

  void store_target_temperature_low_(int idx, esphome::StringRef state) {
    this->target_temperature_low_[idx] = ha_parse_float(state);
  }

  void store_target_temperature_high_(int idx, esphome::StringRef state) {
    this->target_temperature_high_[idx] = ha_parse_float(state);
  }

  void store_current_temperature_(int idx, esphome::StringRef state) {
    this->current_temperature_[idx] = ha_parse_float(state);
  }

  void store_hvac_action_(int idx, esphome::StringRef state) {
    this->hvac_action_[idx] = ha_state_or_unknown(state);
  }

  void store_preset_mode_(int idx, esphome::StringRef state) { this->preset_mode_[idx] = state.str(); }

  void store_preset_modes_(int idx, esphome::StringRef state) {
    this->supports_preset_[idx] = false;

    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    if (doc.is<JsonArray>()) {
      JsonArray preset_modes = doc.as<JsonArray>();
      this->supports_preset_[idx] = !preset_modes.isNull() && preset_modes.size() > 0;
      return;
    }

    this->supports_preset_[idx] = payload != "[]" && !payload.empty();
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(CLIMATE_LIST, CLIMATE_LIST_COUNT, entity_id);
  }

  std::array<std::string, CLIMATE_LIST_COUNT> state_{};
  std::array<std::string, CLIMATE_LIST_COUNT> hvac_action_{};
  std::array<std::string, CLIMATE_LIST_COUNT> preset_mode_{};
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_low_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> target_temperature_high_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<float, CLIMATE_LIST_COUNT> current_temperature_ =
      filled_array<float, CLIMATE_LIST_COUNT>(NAN);
  std::array<bool, CLIMATE_LIST_COUNT> supports_preset_{};
};

using LockStatusTracker = SingleStateTracker<LockEntity, LOCK_LIST_COUNT>;
using SensorStatusTracker = SingleStateTracker<SensorEntity, SENSOR_LIST_COUNT>;

class CoverStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < COVER_LIST_COUNT; i++) {
      const char *entity_id = COVER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&CoverStatusTracker::on_position_, entity_id, "current_position");
    }
    this->request_all_states();
  }

  void request_cover_state(int idx) {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return;
    }

    const char *entity_id = COVER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "current_position", [this, idx](esphome::StringRef state) { this->store_position_(idx, state); });
  }

  void request_all_states() { request_all_states_by_count(COVER_LIST_COUNT, [this](int i) { this->request_cover_state(i); }); }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  bool has_position(int idx) const { return idx >= 0 && idx < COVER_LIST_COUNT && this->has_position_[idx]; }

  float position(int idx) const {
    if (idx < 0 || idx >= COVER_LIST_COUNT) {
      return NAN;
    }
    return this->position_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_position_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_position_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) { this->state_[idx] = ha_state_or_unknown(state); }

  void store_position_(int idx, esphome::StringRef state) {
    float value = ha_parse_float(state);
    this->position_[idx] = value;
    this->has_position_[idx] = !std::isnan(value);
  }

  int find_index_(const std::string &entity_id) const { return find_entity_index(COVER_LIST, COVER_LIST_COUNT, entity_id); }

  std::array<std::string, COVER_LIST_COUNT> state_{};
  std::array<float, COVER_LIST_COUNT> position_ = filled_array<float, COVER_LIST_COUNT>(NAN);
  std::array<bool, COVER_LIST_COUNT> has_position_{};
};

class MediaStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < MEDIA_PLAYER_LIST_COUNT; i++) {
      const char *entity_id = MEDIA_PLAYER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_device_class_, entity_id, "device_class");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_source_list_, entity_id, "source_list");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_volume_, entity_id, "volume_level");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_title_, entity_id, "media_title");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_artist_, entity_id, "media_artist");
      this->subscribe_homeassistant_state(&MediaStatusTracker::on_source_, entity_id, "source");
    }
    this->request_all_states();
  }

  void request_media_state(int idx) {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return;
    }

    const char *entity_id = MEDIA_PLAYER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "device_class", [this, idx](esphome::StringRef state) { this->store_device_class_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "source_list", [this, idx](esphome::StringRef state) { this->store_source_list_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "volume_level", [this, idx](esphome::StringRef state) { this->store_volume_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "media_title", [this, idx](esphome::StringRef state) { this->store_title_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "media_artist", [this, idx](esphome::StringRef state) { this->store_artist_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "source", [this, idx](esphome::StringRef state) { this->store_source_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(MEDIA_PLAYER_LIST_COUNT, [this](int i) { this->request_media_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  const std::string &title(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->title_[idx];
  }

  const std::string &device_class(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->device_class_[idx];
  }

  const std::string &artist(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->artist_[idx];
  }

  const std::string &source(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->source_[idx];
  }

  void set_source(int idx, const std::string &source) {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return;
    }
    this->source_[idx] = source;
  }

  const std::string &source_list(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return empty_string();
    }
    return this->source_list_[idx];
  }

  float volume(int idx) const {
    if (idx < 0 || idx >= MEDIA_PLAYER_LIST_COUNT) {
      return NAN;
    }
    return this->volume_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_volume_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_volume_(idx, state);
    }
  }

  void on_device_class_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_device_class_(idx, state);
    }
  }

  void on_title_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_title_(idx, state);
    }
  }

  void on_artist_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_artist_(idx, state);
    }
  }

  void on_source_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_source_(idx, state);
    }
  }

  void on_source_list_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_source_list_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_title_(int idx, esphome::StringRef state) { this->title_[idx] = state.str(); }

  void store_artist_(int idx, esphome::StringRef state) { this->artist_[idx] = state.str(); }

  void store_source_(int idx, esphome::StringRef state) {
    std::string source = state.str();
    std::string device_class = this->device_class_[idx];
    if ((device_class == "tv" || device_class == "receiver") && trim_copy(source).empty()) {
      return;
    }
    this->source_[idx] = source;
  }

  void store_device_class_(int idx, esphome::StringRef state) { this->device_class_[idx] = state.str(); }

  void store_source_list_(int idx, esphome::StringRef state) {
    this->source_list_[idx].clear();
    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    if (!doc.is<JsonArray>()) {
      return;
    }

    JsonArray source_list = doc.as<JsonArray>();
    bool first = true;
    for (JsonVariant value : source_list) {
      std::string source = trim_copy(value.as<std::string>());
      if (source.empty()) {
        continue;
      }
      if (!first) {
        this->source_list_[idx] += "|";
      }
      this->source_list_[idx] += source;
      first = false;
    }
  }

  void store_volume_(int idx, esphome::StringRef state) {
    this->volume_[idx] = ha_parse_float(state);
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(MEDIA_PLAYER_LIST, MEDIA_PLAYER_LIST_COUNT, entity_id);
  }

  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> state_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> device_class_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> title_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> artist_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> source_{};
  std::array<std::string, MEDIA_PLAYER_LIST_COUNT> source_list_{};
  std::array<float, MEDIA_PLAYER_LIST_COUNT> volume_ = filled_array<float, MEDIA_PLAYER_LIST_COUNT>(NAN);
};

using AutomationStatusTracker = SingleStateTracker<AutomationEntity, AUTOMATION_LIST_COUNT>;
using AlarmStatusTracker = SingleStateTracker<AlarmEntity, ALARM_LIST_COUNT>;

class WeatherStatusTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    for (int i = 0; i < WEATHER_LIST_COUNT; i++) {
      const char *entity_id = WEATHER_LIST[i].entity_id;
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_state_, entity_id);
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_temperature_, entity_id, "temperature");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_humidity_, entity_id, "humidity");
      this->subscribe_homeassistant_state(&WeatherStatusTracker::on_forecast_, entity_id, "forecast");
    }
    this->request_all_states();
  }

  void request_weather_state(int idx) {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return;
    }

    const char *entity_id = WEATHER_LIST[idx].entity_id;
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "", [this, idx](esphome::StringRef state) { this->store_state_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "temperature",
        [this, idx](esphome::StringRef state) { this->store_temperature_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "humidity", [this, idx](esphome::StringRef state) { this->store_humidity_(idx, state); });
    esphome::api::global_api_server->get_home_assistant_state(
        entity_id, "forecast", [this, idx](esphome::StringRef state) { this->store_forecast_(idx, state); });
  }

  void request_all_states() {
    request_all_states_by_count(WEATHER_LIST_COUNT, [this](int i) { this->request_weather_state(i); });
  }

  const std::string &state(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return unknown_string();
    }
    return this->state_[idx];
  }

  float temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->temperature_[idx];
  }

  float humidity(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->humidity_[idx];
  }

  float high_temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->high_temperature_[idx];
  }

  float low_temperature(int idx) const {
    if (idx < 0 || idx >= WEATHER_LIST_COUNT) {
      return NAN;
    }
    return this->low_temperature_[idx];
  }

 protected:
  void on_state_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_state_(idx, state);
    }
  }

  void on_temperature_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_temperature_(idx, state);
    }
  }

  void on_humidity_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_humidity_(idx, state);
    }
  }

  void on_forecast_(const std::string &entity_id, esphome::StringRef state) {
    int idx = this->find_index_(entity_id);
    if (idx >= 0) {
      this->store_forecast_(idx, state);
    }
  }

  void store_state_(int idx, esphome::StringRef state) {
    this->state_[idx] = ha_state_or_unknown(state);
  }

  void store_temperature_(int idx, esphome::StringRef state) {
    this->temperature_[idx] = ha_parse_float(state);
  }

  void store_humidity_(int idx, esphome::StringRef state) {
    this->humidity_[idx] = ha_parse_float(state);
  }

  void store_forecast_(int idx, esphome::StringRef state) {
    this->high_temperature_[idx] = NAN;
    this->low_temperature_[idx] = NAN;

    std::string payload = state.str();
    if (ha_state_missing(payload)) {
      return;
    }

    JsonDocument doc = esphome::json::parse_json(payload);
    JsonArray forecast;
    if (doc.is<JsonArray>()) {
      forecast = doc.as<JsonArray>();
    } else if (doc.is<JsonObject>()) {
      JsonObject root = doc.as<JsonObject>();
      forecast = root["forecast"].as<JsonArray>();
    } else {
      return;
    }

    if (forecast.isNull() || forecast.size() == 0) {
      return;
    }

    JsonObject today = forecast[0].as<JsonObject>();
    if (today.isNull()) {
      return;
    }

    auto read_number = [&](JsonObject obj, const char *key) -> float {
      if (obj[key].is<float>() || obj[key].is<int>()) {
        return obj[key].as<float>();
      }
      if (obj[key].is<const char *>()) {
        const char *value = obj[key].as<const char *>();
        if (value != nullptr && value[0] != '\0') {
          return strtof(value, nullptr);
        }
      }
      return NAN;
    };

    float high = read_number(today, "temperature");
    if (std::isnan(high)) high = read_number(today, "temperature_high");
    if (std::isnan(high)) high = read_number(today, "native_temperature");
    if (std::isnan(high)) high = read_number(today, "native_temperature_high");

    float low = read_number(today, "templow");
    if (std::isnan(low)) low = read_number(today, "temperature_low");
    if (std::isnan(low)) low = read_number(today, "native_templow");
    if (std::isnan(low)) low = read_number(today, "native_temperature_low");

    this->high_temperature_[idx] = high;
    this->low_temperature_[idx] = low;
  }

  int find_index_(const std::string &entity_id) const {
    return find_entity_index(WEATHER_LIST, WEATHER_LIST_COUNT, entity_id);
  }

  std::array<std::string, WEATHER_LIST_COUNT> state_{};
  std::array<float, WEATHER_LIST_COUNT> temperature_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> humidity_ = filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> high_temperature_ =
      filled_array<float, WEATHER_LIST_COUNT>(NAN);
  std::array<float, WEATHER_LIST_COUNT> low_temperature_ =
      filled_array<float, WEATHER_LIST_COUNT>(NAN);
};

class NotificationFeedTracker : public esphome::api::CustomAPIDevice {
 public:
  void setup() {
    if (notification_feed_entity_()[0] == '\0') {
      return;
    }
    this->subscribe_homeassistant_state(
        &NotificationFeedTracker::on_payload_, notification_feed_entity_(),
        notification_feed_attribute_());
    this->subscribe_homeassistant_state(
        &NotificationFeedTracker::on_ids_payload_, notification_feed_entity_(),
        notification_feed_ids_attribute_());
    this->request_notifications();
  }

  void request_notifications() {
    if (notification_feed_entity_()[0] == '\0') {
      this->clear_();
      return;
    }

    esphome::api::global_api_server->get_home_assistant_state(
        notification_feed_entity_(), notification_feed_attribute_(),
        [this](esphome::StringRef state) { this->on_payload_(state); });
    esphome::api::global_api_server->get_home_assistant_state(
        notification_feed_entity_(), notification_feed_ids_attribute_(),
        [this](esphome::StringRef state) { this->on_ids_payload_(state); });
  }

  int item_count() const { return this->count_ > 0 ? this->count_ : 1; }

  std::string label(int idx) const {
    if (this->count_ <= 0) {
      return ""; // NO ALERTS HEADER
    }
    idx = clamp_mode_index(idx, this->count_);
    return std::to_string(idx + 1) + " OF " + std::to_string(this->count_);
  }

  std::string entity() const { return notification_feed_entity_(); }

  const std::string &message(int idx) const {
    if (this->count_ <= 0) {
      return empty_string();
    }
    idx = clamp_mode_index(idx, this->count_);
    return this->messages_[idx];
  }

  const std::string &notification_id(int idx) const {
    if (this->count_ <= 0) {
      return empty_string();
    }
    idx = clamp_mode_index(idx, this->count_);
    return this->ids_[idx];
  }

 private:
  static std::string trim_copy_(const std::string &value) {
    size_t start = 0;
    size_t end = value.size();
    while (start < end && (value[start] == ' ' || value[start] == '\n' || value[start] == '\r' || value[start] == '\t')) {
      start++;
    }
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\n' || value[end - 1] == '\r' || value[end - 1] == '\t')) {
      end--;
    }
    return value.substr(start, end - start);
  }

  static const char *notification_feed_entity_() {
    return NOTIFICATION_FEED_ENTITY;
  }

  static const char *notification_feed_attribute_() {
    return (NOTIFICATION_FEED_ATTRIBUTE[0] == '\0') ? "messages" : NOTIFICATION_FEED_ATTRIBUTE;
  }

  static const char *notification_feed_ids_attribute_() {
    return (NOTIFICATION_FEED_IDS_ATTRIBUTE[0] == '\0') ? "ids" : NOTIFICATION_FEED_IDS_ATTRIBUTE;
  }

  static std::string notification_feed_separator_() {
    std::string separator = NOTIFICATION_FEED_SEPARATOR;
    return separator.empty() ? std::string("||") : separator;
  }

  void clear_() {
    this->count_ = 0;
    for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
      this->messages_[i].clear();
      this->ids_[i].clear();
    }
  }

  void on_payload_(esphome::StringRef state) {
    std::string payload = trim_copy_(state.str());
    this->clear_();
    if (ha_state_missing(payload) || payload == "[]" || payload == "none") {
      return;
    }

    const std::string separator = notification_feed_separator_();
    size_t start = 0;
    while (start <= payload.size() && this->count_ < NOTIFICATION_FEED_MAX_ITEMS) {
      size_t end = payload.find(separator, start);
      std::string item = trim_copy_(payload.substr(start, end == std::string::npos ? std::string::npos : end - start));
      if (!item.empty()) {
        this->messages_[this->count_++] = item;
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + separator.size();
    }
  }

  void on_ids_payload_(esphome::StringRef state) {
    std::string payload = trim_copy_(state.str());
    for (int i = 0; i < NOTIFICATION_FEED_MAX_ITEMS; i++) {
      this->ids_[i].clear();
    }
    if (ha_state_missing(payload) || payload == "[]" || payload == "none") {
      return;
    }

    const std::string separator = notification_feed_separator_();
    size_t start = 0;
    int idx = 0;
    while (start <= payload.size() && idx < NOTIFICATION_FEED_MAX_ITEMS) {
      size_t end = payload.find(separator, start);
      std::string item = trim_copy_(payload.substr(start, end == std::string::npos ? std::string::npos : end - start));
      if (!item.empty()) {
        this->ids_[idx++] = item;
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + separator.size();
    }
  }

  int count_{0};
  std::string messages_[NOTIFICATION_FEED_MAX_ITEMS];
  std::string ids_[NOTIFICATION_FEED_MAX_ITEMS];
};

static LightStatusTracker light_status_tracker_storage;
static SwitchStatusTracker switch_status_tracker_storage(SWITCH_LIST);
static FanStatusTracker fan_status_tracker_storage;
static HumidifierStatusTracker humidifier_status_tracker_storage;
static ClimateStatusTracker climate_status_tracker_storage;
static LockStatusTracker lock_status_tracker_storage(LOCK_LIST);
static CoverStatusTracker cover_status_tracker_storage;
static MediaStatusTracker media_status_tracker_storage;
static SensorStatusTracker sensor_status_tracker_storage(SENSOR_LIST);
static AutomationStatusTracker automation_status_tracker_storage(AUTOMATION_LIST);
static AlarmStatusTracker alarm_status_tracker_storage(ALARM_LIST);
static NotificationFeedTracker notification_feed_tracker_storage;
static WeatherStatusTracker weather_status_tracker_storage;
static bool remote_status_trackers_initialized = false;
static RemoteMode last_requested_mode = REMOTE_MODE_INFO;
static int last_requested_mode_index = -1;
static uint32_t last_requested_mode_at = 0;

static inline void ensure_remote_status_trackers() {
  if (remote_status_trackers_initialized) {
    return;
  }
  light_status_tracker_storage.setup();
  switch_status_tracker_storage.setup();
  fan_status_tracker_storage.setup();
  humidifier_status_tracker_storage.setup();
  climate_status_tracker_storage.setup();
  lock_status_tracker_storage.setup();
  cover_status_tracker_storage.setup();
  media_status_tracker_storage.setup();
  sensor_status_tracker_storage.setup();
  automation_status_tracker_storage.setup();
  alarm_status_tracker_storage.setup();
  notification_feed_tracker_storage.setup();
  weather_status_tracker_storage.setup();
  remote_status_trackers_initialized = true;
}

static inline void ensure_light_status_tracker() { ensure_remote_status_trackers(); }

static inline void request_selected_light_status(int idx) {
  ensure_remote_status_trackers();
  light_status_tracker_storage.request_light_state(idx);
}

static inline const std::string &selected_light_state(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.state(idx);
}

static inline bool selected_light_has_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.has_brightness(idx);
}

static inline float selected_light_brightness(int idx) {
  ensure_remote_status_trackers();
  return light_status_tracker_storage.brightness(idx);
}

static inline void request_selected_switch_status(int idx) {
  ensure_remote_status_trackers();
  switch_status_tracker_storage.request_state(idx);
}

static inline const std::string &selected_switch_state(int idx) {
  ensure_remote_status_trackers();
  return switch_status_tracker_storage.state(idx);
}

static inline void request_selected_fan_status(int idx) {
  ensure_remote_status_trackers();
  fan_status_tracker_storage.request_fan_state(idx);
}

static inline const std::string &selected_fan_state(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.state(idx);
}

static inline bool selected_fan_has_percentage(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.has_percentage(idx);
}

static inline float selected_fan_percentage(int idx) {
  ensure_remote_status_trackers();
  return fan_status_tracker_storage.percentage(idx);
}

static inline void request_selected_humidifier_status(int idx) {
  ensure_remote_status_trackers();
  humidifier_status_tracker_storage.request_humidifier_state(idx);
}

static inline const std::string &selected_humidifier_state(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.state(idx);
}

static inline const std::string &humidifier_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.action(idx);
}

static inline const std::string &humidifier_mode_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.mode(idx);
}

static inline const std::string &humidifier_available_modes_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.available_modes(idx);
}

static inline std::string next_humidifier_mode_for_index(int idx) {
  std::string current_mode = trim_copy(humidifier_mode_for_index(idx));
  std::string available_modes = humidifier_available_modes_for_index(idx);
  int count = delimited_option_count(available_modes);
  if (count <= 0) {
    return "";
  }
  for (int i = 0; i < count; i++) {
    if (delimited_option_at(available_modes, i) == current_mode) {
      return delimited_option_at(available_modes, (i + 1) % count);
    }
  }
  return delimited_option_at(available_modes, 0);
}

static inline float humidifier_target_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.target_humidity(idx);
}

static inline float humidifier_current_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return humidifier_status_tracker_storage.current_humidity(idx);
}

static inline void request_selected_climate_status(int idx) {
  ensure_remote_status_trackers();
  climate_status_tracker_storage.request_climate_state(idx);
}

static inline const std::string &selected_climate_state(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.state(idx);
}

static inline const std::string &climate_hvac_action_for_index(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.hvac_action(idx);
}

static inline const std::string &selected_climate_preset_mode(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.preset_mode(idx);
}

static inline bool climate_supports_preset(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.supports_preset(idx);
}

static inline float selected_climate_target_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature(idx);
}

static inline float selected_climate_target_temperature_low(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_low(idx);
}

static inline float selected_climate_target_temperature_high(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.target_temperature_high(idx);
}

static inline float selected_climate_current_temperature(int idx) {
  ensure_remote_status_trackers();
  return climate_status_tracker_storage.current_temperature(idx);
}

static inline void request_selected_lock_status(int idx) {
  ensure_remote_status_trackers();
  lock_status_tracker_storage.request_state(idx);
}

static inline const std::string &selected_lock_state(int idx) {
  ensure_remote_status_trackers();
  return lock_status_tracker_storage.state(idx);
}

static inline void request_selected_cover_status(int idx) {
  ensure_remote_status_trackers();
  cover_status_tracker_storage.request_cover_state(idx);
}

static inline const std::string &selected_cover_state(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.state(idx);
}

static inline float selected_cover_position(int idx) {
  ensure_remote_status_trackers();
  return cover_status_tracker_storage.position(idx);
}

static inline void request_selected_media_status(int idx) {
  ensure_remote_status_trackers();
  media_status_tracker_storage.request_media_state(idx);
}

static inline const std::string &selected_media_state(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.state(idx);
}

static inline const std::string &media_title_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.title(idx);
}

static inline const std::string &media_device_class_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.device_class(idx);
}

static inline const std::string &media_source_list_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source_list(idx);
}

static inline void set_media_source_for_index(int idx, const std::string &source) {
  ensure_remote_status_trackers();
  media_status_tracker_storage.set_source(idx, source);
}

static inline const std::string &media_artist_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.artist(idx);
}

static inline const std::string &media_source_for_index(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.source(idx);
}

static inline float selected_media_volume(int idx) {
  ensure_remote_status_trackers();
  return media_status_tracker_storage.volume(idx);
}

static inline void request_selected_sensor_status(int idx) {
  ensure_remote_status_trackers();
  sensor_status_tracker_storage.request_state(idx);
}

static inline const std::string &sensor_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return sensor_status_tracker_storage.state(idx);
}

static inline void request_selected_automation_status(int idx) {
  ensure_remote_status_trackers();
  automation_status_tracker_storage.request_state(idx);
}

static inline const std::string &automation_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return automation_status_tracker_storage.state(idx);
}

static inline void request_selected_alarm_status(int idx) {
  ensure_remote_status_trackers();
  alarm_status_tracker_storage.request_state(idx);
}

static inline const std::string &alarm_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return alarm_status_tracker_storage.state(idx);
}

static inline void request_selected_notification_status(int idx) {
  (void) idx;
  if (!notifications_mode_enabled()) {
    return;
  }
  ensure_remote_status_trackers();
  notification_feed_tracker_storage.request_notifications();
}

static inline int notification_mode_item_count() {
  if (!notifications_mode_enabled()) {
    return 0;
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.item_count();
}

static inline std::string notification_mode_item_name(int idx) {
  if (!notifications_mode_enabled()) {
    return "";
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.label(idx);
}

static inline std::string notification_mode_item_entity(int idx) {
  (void) idx;
  if (!notifications_mode_enabled()) {
    return "";
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.entity();
}

static inline const std::string &notification_message_for_index(int idx) {
  if (!notifications_mode_enabled()) {
    return empty_string();
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.message(idx);
}

static inline const std::string &notification_id_for_index(int idx) {
  if (!notifications_mode_enabled()) {
    return empty_string();
  }
  ensure_remote_status_trackers();
  return notification_feed_tracker_storage.notification_id(idx);
}

static inline void request_selected_weather_status(int idx) {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.request_weather_state(idx);
}

static inline void request_all_weather_status() {
  ensure_remote_status_trackers();
  weather_status_tracker_storage.request_all_states();
}

static inline const std::string &weather_state_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.state(idx);
}

static inline float weather_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.temperature(idx);
}

static inline float weather_humidity_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.humidity(idx);
}

static inline float weather_high_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.high_temperature(idx);
}

static inline float weather_low_temperature_for_index(int idx) {
  ensure_remote_status_trackers();
  return weather_status_tracker_storage.low_temperature(idx);
}

static inline void request_mode_status(RemoteMode mode, int idx) {
  switch (mode) {
    case REMOTE_MODE_LIGHTS:
      request_selected_light_status(idx);
      break;
    case REMOTE_MODE_SWITCHES:
      request_selected_switch_status(idx);
      break;
    case REMOTE_MODE_FANS:
      request_selected_fan_status(idx);
      break;
    case REMOTE_MODE_HUMIDIFIERS:
      request_selected_humidifier_status(idx);
      break;
    case REMOTE_MODE_CLIMATE:
      request_selected_climate_status(idx);
      break;
    case REMOTE_MODE_LOCKS:
      request_selected_lock_status(idx);
      break;
    case REMOTE_MODE_COVERS:
      request_selected_cover_status(idx);
      break;
    case REMOTE_MODE_MEDIA:
      request_selected_media_status(idx);
      break;
    case REMOTE_MODE_SENSORS:
      request_selected_sensor_status(idx);
      break;
    case REMOTE_MODE_AUTOMATION:
      request_selected_automation_status(idx);
      break;
    case REMOTE_MODE_ALARMS:
      request_selected_alarm_status(idx);
      break;
    case REMOTE_MODE_NOTIFICATIONS:
      request_selected_notification_status(idx);
      break;
    case REMOTE_MODE_WEATHER:
      request_selected_weather_status(idx);
      break;
    case REMOTE_MODE_INFO:
      break;
  }
}

static inline void request_mode_status_throttled(
    RemoteMode mode, int idx, uint32_t now, uint32_t min_interval_ms, bool force = false) {
  if (!force && last_requested_mode == mode && last_requested_mode_index == idx &&
      (now - last_requested_mode_at) < min_interval_ms) {
    return;
  }

  request_mode_status(mode, idx);
  last_requested_mode = mode;
  last_requested_mode_index = idx;
  last_requested_mode_at = now;
}
