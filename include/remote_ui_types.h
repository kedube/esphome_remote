#pragma once

// UI-facing enums and pure string helpers. Kept free of ESPHome API, JSON and
// tracker dependencies so the renderer (and the host-side UI preview tool) can
// include it on its own.

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

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

// Length of text[0, len) without a trailing multi-byte UTF-8 character that
// truncation cut short. ESPHome's font code reads a whole character once it
// sees the first byte, so a cut one would make it read past the terminator.
inline size_t utf8_complete_length(const char *text, size_t len) {
  size_t start = len;
  while (start > 0 && (static_cast<unsigned char>(text[start - 1]) & 0xC0) == 0x80) {
    start--;
  }
  if (start == 0) {
    return len;
  }
  auto lead = static_cast<unsigned char>(text[start - 1]);
  size_t needed = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
  return len - (start - 1) < needed ? start - 1 : len;
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
  write_idx = utf8_complete_length(buffer, write_idx);
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
  write_idx = utf8_complete_length(buffer, write_idx);
  buffer[write_idx] = '\0';
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

inline bool remote_ui_has_dual_climate_target(
    const std::string &selected_item_state, float target_temp_low, float target_temp_high) {
  return !std::isnan(target_temp_low) &&
         !std::isnan(target_temp_high) &&
         (selected_item_state == "heat_cool" || target_temp_low != target_temp_high);
}

// Formats the climate/water-heater target text: "LOW: 68°F   HIGH: 74°F" when
// dual_target, otherwise "TARGET: 70°F".
inline void format_climate_target_detail(
    char *buffer, size_t buffer_size, bool dual_target, float low, float high, float single_target,
    const char *temperature_unit) {
  if (dual_target) {
    snprintf(buffer, buffer_size, "LOW: %.0f°%s   HIGH: %.0f°%s", low, temperature_unit, high, temperature_unit);
  } else {
    snprintf(buffer, buffer_size, "TARGET: %.0f°%s", single_target, temperature_unit);
  }
}
