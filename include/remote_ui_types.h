#pragma once

// UI-facing enums and pure string helpers. Kept free of ESPHome API, JSON and
// tracker dependencies so the renderer (and the host-side UI preview tool) can
// include it on its own.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

// The large text for an item without a usable state. The trackers keep ""
// until Home Assistant has sent anything, and store "unknown" when it reports
// that (an entity that hasn't produced a value since Home Assistant started).
inline const char *missing_state_word(const std::string &state) {
  if (state == "unavailable") {
    return "UNAVAILABLE";
  }
  if (state == "unknown" || state == "None") {
    return "UNKNOWN";
  }
  return "SYNCING";
}

// Whole Home Assistant number such as "23.6000003814697", "-4" or "1e-05".
// Anything else, including "nan", "inf" and numbers followed by text, is not.
inline bool parse_ha_number(const std::string &text, double *out) {
  const char *begin = text.c_str();
  while (*begin == ' ' || *begin == '\t') {
    begin++;
  }
  const char *end = begin;
  while (*end != '\0' && strchr("0123456789+-.eE", *end) != nullptr) {
    end++;
  }
  const char *rest = end;
  while (*rest == ' ' || *rest == '\t') {
    rest++;
  }
  if (end == begin || *rest != '\0') {
    return false;
  }
  std::string number(begin, end - begin);
  char *parsed_end = nullptr;
  double value = strtod(number.c_str(), &parsed_end);
  if (parsed_end != number.c_str() + number.size() || !std::isfinite(value)) {
    return false;
  }
  *out = value;
  return true;
}

// A reading with the fewest decimals, up to three, that still show it exactly.
// Home Assistant keeps 15 significant digits of a sensor's float, so an ESPHome
// sensor at 23.6 arrives as "23.6000003814697": this prints "23.6". Readings
// that need more decimals keep six significant digits.
inline void format_ha_number(double value, char *buf, size_t size) {
  double magnitude = std::fabs(value);
  double tolerance = 1e-6 * (magnitude > 1.0 ? magnitude : 1.0);
  double scale = 1.0;
  for (int decimals = 0; decimals <= 3; decimals++, scale *= 10.0) {
    double rounded = std::round(value * scale) / scale;
    if (std::fabs(rounded - value) <= tolerance) {
      snprintf(buf, size, "%.*f", decimals, rounded == 0.0 ? 0.0 : rounded);
      return;
    }
  }
  snprintf(buf, size, "%.6g", value);
}

// value rounded to decimals places, without the sign printf gives a negative
// value that rounds to zero ("-0°").
inline double display_rounded(double value, int decimals) {
  double scale = std::pow(10.0, decimals);
  double rounded = std::round(value * scale) / scale;
  return rounded == 0.0 ? 0.0 : rounded;
}

// Home Assistant writes the micro sign as Greek mu (U+03BC) and uses a few
// other characters outside Latin-1 in units ("μg/m³", "BTU/(h⋅ft²)", "inH₂O").
// The fonts carry Latin-1, which has a look-alike for each of them.
inline void normalize_unit_text(std::string &unit) {
  static const char MU[] = {'\xCE', '\xBC', '\0'};             // U+03BC
  static const char MICRO[] = {'\xC2', '\xB5', '\0'};          // U+00B5
  static const char DOT_OPERATOR[] = {'\xE2', '\x8B', '\x85', '\0'};  // U+22C5
  static const char MIDDLE_DOT[] = {'\xC2', '\xB7', '\0'};     // U+00B7
  size_t pos = 0;
  while ((pos = unit.find(MU, pos)) != std::string::npos) {
    unit.replace(pos, 2, MICRO);
    pos += 2;
  }
  pos = 0;
  while ((pos = unit.find(DOT_OPERATOR, pos)) != std::string::npos) {
    unit.replace(pos, 3, MIDDLE_DOT);
    pos += 2;
  }
  // Subscript digits U+2080-2089 ("H₂O") become plain digits.
  for (pos = 0; pos + 2 < unit.size(); pos++) {
    if (unit[pos] == '\xE2' && unit[pos + 1] == '\x82' && static_cast<unsigned char>(unit[pos + 2]) >= 0x80 &&
        static_cast<unsigned char>(unit[pos + 2]) <= 0x89) {
      char digit = static_cast<char>('0' + (static_cast<unsigned char>(unit[pos + 2]) - 0x80));
      unit.replace(pos, 3, 1, digit);
    }
  }
}

// The percentage one speed above (direction > 0) or below the current one, as
// Home Assistant's fan.increase_speed and fan.decrease_speed work it out; 0
// means off. step_pct is the fan's percentage_step (33.3 on a 3-speed fan).
// Sending the result with fan.set_percentage, rather than asking Home
// Assistant to step, keeps a second press from stepping from a stale value.
inline int fan_step_percentage(int current_pct, float step_pct, int direction) {
  int speeds = std::isfinite(step_pct) && step_pct >= 1.0f ? static_cast<int>(std::lround(100.0f / step_pct)) : 100;
  speeds = speeds < 1 ? 1 : speeds > 100 ? 100 : speeds;
  int current = current_pct < 0 ? 0 : current_pct > 100 ? 100 : current_pct;
  int speed = (current * speeds + 99) / 100;  // Home Assistant rounds a percentage up to a speed
  speed += direction > 0 ? 1 : -1;
  if (speed <= 0) {
    return 0;
  }
  if (speed > speeds) {
    speed = speeds;
  }
  return speed * 100 / speeds;
}

// A light's supported_color_modes ('|'-joined): -1 until they arrive, 0 for a
// light that can only switch on and off, 1 for one that dims.
inline int light_modes_dimmable(const std::string &modes) {
  if (modes.empty()) {
    return -1;
  }
  size_t start = 0;
  while (start <= modes.size()) {
    size_t end = modes.find('|', start);
    std::string mode = modes.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!mode.empty() && mode != "onoff" && mode != "unknown") {
      return 1;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return 0;
}

// Degrees for a compass point ("NW", "ssw"). Some weather integrations report
// the wind's direction that way instead of in degrees.
inline float compass_point_degrees(const char *text, size_t len) {
  static const char *const POINTS[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                       "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  while (len > 0 && (*text == ' ' || *text == '\t')) {
    text++;
    len--;
  }
  while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t')) {
    len--;
  }
  if (len == 0 || len > 3) {
    return NAN;
  }
  char upper[4];
  for (size_t i = 0; i < len; i++) {
    upper[i] = (text[i] >= 'a' && text[i] <= 'z') ? static_cast<char>(text[i] - 'a' + 'A') : text[i];
  }
  upper[len] = '\0';
  for (int i = 0; i < 16; i++) {
    if (strcmp(upper, POINTS[i]) == 0) {
      return i * 22.5f;
    }
  }
  return NAN;
}

// Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's
// days_from_civil).
inline int64_t days_from_civil(int64_t y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

// Reads the timestamp a timestamp sensor reports ("2026-10-02T21:30:00+00:00",
// with optional fractional seconds or "Z"). Fills epoch with its UTC seconds.
// A date on its own ("2026-10-02", a date sensor) sets date_only.
inline bool parse_ha_timestamp(const std::string &text, int64_t *epoch, bool *date_only) {
  int year, month, day;
  int used = 0;
  if (sscanf(text.c_str(), "%4d-%2d-%2d%n", &year, &month, &day, &used) != 3 || used != 10 || month < 1 ||
      month > 12 || day < 1 || day > 31) {
    return false;
  }
  int64_t days = days_from_civil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  if (text.size() == 10) {
    *epoch = days * 86400;
    *date_only = true;
    return true;
  }
  const char *p = text.c_str() + 10;
  if (*p != 'T' && *p != ' ') {
    return false;
  }
  int hour, minute, second = 0;
  used = 0;
  if (sscanf(p + 1, "%2d:%2d%n", &hour, &minute, &used) != 2 || used != 5 || hour > 23 || minute > 59) {
    return false;
  }
  p += 1 + used;
  if (*p == ':') {
    used = 0;
    if (sscanf(p + 1, "%2d%n", &second, &used) != 1 || used != 2 || second > 60) {
      return false;
    }
    p += 3;
    if (*p == '.') {
      p++;
      while (*p >= '0' && *p <= '9') {
        p++;
      }
    }
  }
  int offset = 0;
  if (*p == 'Z') {
    p++;
  } else if (*p == '+' || *p == '-') {
    int sign = *p == '-' ? -1 : 1;
    int off_h, off_m = 0;
    used = 0;
    if (sscanf(p + 1, "%2d:%2d%n", &off_h, &off_m, &used) == 2 && used == 5) {
      p += 6;
    } else if (sscanf(p + 1, "%2d%2d%n", &off_h, &off_m, &used) == 2 && used == 4) {
      p += 5;
    } else {
      return false;
    }
    offset = sign * (off_h * 3600 + off_m * 60);
  } else {
    // No zone: Home Assistant always writes one, so this isn't its timestamp.
    return false;
  }
  if (*p != '\0') {
    return false;
  }
  *epoch = days * 86400 + hour * 3600 + minute * 60 + second - offset;
  *date_only = false;
  return true;
}

// Decimal places for a weather reading in the unit Home Assistant reports it
// in: "29.92 inHg" needs two, "101.3 kPa" one, "1013 hPa" none.
inline int weather_pressure_decimals(const char *unit) {
  if (strcmp(unit, "inHg") == 0) return 2;
  if (strcmp(unit, "kPa") == 0 || strcmp(unit, "psi") == 0) return 1;
  return 0;
}

inline int weather_speed_decimals(const char *unit) { return strcmp(unit, "m/s") == 0 ? 1 : 0; }

inline int weather_precipitation_decimals(const char *unit) { return strcmp(unit, "in") == 0 ? 2 : 1; }

// "F" for Home Assistant's "°F"; the remote prints the degree sign itself.
inline const char *temperature_unit_letter(const std::string &unit, const char *fallback) {
  if (unit == "\xC2\xB0" "F" || unit == "F") return "F";
  if (unit == "\xC2\xB0" "C" || unit == "C") return "C";
  return fallback;
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

  if (write_idx == 0) {
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

inline const char *alarm_arm_mode_short_label(AlarmArmMode mode) {
  switch (mode) {
    case ALARM_ARM_MODE_HOME:
      return "HOME";
    case ALARM_ARM_MODE_NIGHT:
      return "NIGHT";
    case ALARM_ARM_MODE_VACATION:
      return "VAC";
    case ALARM_ARM_MODE_AWAY:
    default:
      return "AWAY";
  }
}

// alarm_control_panel supported_features bits. A panel's features are -1
// until Home Assistant has sent them, and then everything is offered.
inline constexpr int ALARM_FEATURE_TRIGGER = 8;

inline int alarm_arm_mode_feature(AlarmArmMode mode) {
  switch (mode) {
    case ALARM_ARM_MODE_HOME:
      return 1;
    case ALARM_ARM_MODE_NIGHT:
      return 4;
    case ALARM_ARM_MODE_VACATION:
      return 32;
    case ALARM_ARM_MODE_AWAY:
    default:
      return 2;
  }
}

inline bool alarm_arm_mode_supported(int features, AlarmArmMode mode) {
  return features < 0 || (features & alarm_arm_mode_feature(mode)) != 0;
}

inline bool alarm_trigger_supported(int features) { return features < 0 || (features & ALARM_FEATURE_TRIGGER) != 0; }

// The arm mode the footer offers: the one picked, or the first the panel
// supports when it doesn't support that one.
inline AlarmArmMode alarm_effective_arm_mode(int selected, int features) {
  AlarmArmMode mode = clamp_alarm_arm_mode(selected);
  if (alarm_arm_mode_supported(features, mode)) {
    return mode;
  }
  for (int i = 0; i < ALARM_ARM_MODE_COUNT; i++) {
    if (alarm_arm_mode_supported(features, static_cast<AlarmArmMode>(i))) {
      return static_cast<AlarmArmMode>(i);
    }
  }
  return mode;
}

// Plus/Minus in the footer's arm-mode picker: the next mode the panel supports.
inline AlarmArmMode next_supported_alarm_arm_mode(AlarmArmMode mode, int step, int features) {
  AlarmArmMode next = mode;
  for (int i = 0; i < ALARM_ARM_MODE_COUNT; i++) {
    next = next_alarm_arm_mode(next, step >= 0 ? 1 : -1);
    if (alarm_arm_mode_supported(features, next)) {
      return next;
    }
  }
  return mode;
}

// What the remote asks an alarm panel to do. The script that sends it takes
// one int, the panel's index times ALARM_REQUEST_STRIDE plus the request.
enum AlarmRequest {
  ALARM_REQUEST_DISARM = 0,
  ALARM_REQUEST_ARM_AWAY = 1,
  ALARM_REQUEST_ARM_HOME = 2,
  ALARM_REQUEST_ARM_NIGHT = 3,
  ALARM_REQUEST_ARM_VACATION = 4,
  ALARM_REQUEST_TRIGGER = 5,
};

inline constexpr int ALARM_REQUEST_STRIDE = 8;

inline AlarmRequest alarm_request_for_arm_mode(AlarmArmMode mode) {
  switch (mode) {
    case ALARM_ARM_MODE_HOME:
      return ALARM_REQUEST_ARM_HOME;
    case ALARM_ARM_MODE_NIGHT:
      return ALARM_REQUEST_ARM_NIGHT;
    case ALARM_ARM_MODE_VACATION:
      return ALARM_REQUEST_ARM_VACATION;
    case ALARM_ARM_MODE_AWAY:
    default:
      return ALARM_REQUEST_ARM_AWAY;
  }
}

inline int encode_alarm_request(int alarm_index, AlarmRequest request) {
  return alarm_index * ALARM_REQUEST_STRIDE + static_cast<int>(request);
}

inline int alarm_request_index(int code) { return code / ALARM_REQUEST_STRIDE; }

inline AlarmRequest alarm_request_kind(int code) {
  int kind = code % ALARM_REQUEST_STRIDE;
  return kind >= ALARM_REQUEST_DISARM && kind <= ALARM_REQUEST_TRIGGER ? static_cast<AlarmRequest>(kind)
                                                                     : ALARM_REQUEST_DISARM;
}

inline const char *alarm_request_action(AlarmRequest request) {
  switch (request) {
    case ALARM_REQUEST_ARM_AWAY:
      return "alarm_control_panel.alarm_arm_away";
    case ALARM_REQUEST_ARM_HOME:
      return "alarm_control_panel.alarm_arm_home";
    case ALARM_REQUEST_ARM_NIGHT:
      return "alarm_control_panel.alarm_arm_night";
    case ALARM_REQUEST_ARM_VACATION:
      return "alarm_control_panel.alarm_arm_vacation";
    case ALARM_REQUEST_TRIGGER:
      return "alarm_control_panel.alarm_trigger";
    case ALARM_REQUEST_DISARM:
    default:
      return "alarm_control_panel.alarm_disarm";
  }
}

inline const char *alarm_request_expected_state(AlarmRequest request) {
  switch (request) {
    case ALARM_REQUEST_ARM_AWAY:
      return "armed_away";
    case ALARM_REQUEST_ARM_HOME:
      return "armed_home";
    case ALARM_REQUEST_ARM_NIGHT:
      return "armed_night";
    case ALARM_REQUEST_ARM_VACATION:
      return "armed_vacation";
    case ALARM_REQUEST_TRIGGER:
      return "triggered";
    case ALARM_REQUEST_DISARM:
    default:
      return "disarmed";
  }
}

inline const char *alarm_request_progress_feedback(AlarmRequest request) {
  switch (request) {
    case ALARM_REQUEST_DISARM:
      return "DISARMING...";
    case ALARM_REQUEST_TRIGGER:
      return "TRIGGERING...";
    default:
      return "ARMING...";
  }
}

// The state shown while a request is on its way: "arming" until Home
// Assistant reports otherwise. A trigger shows the panel's state unchanged.
inline const char *alarm_request_transition_state(AlarmRequest request) {
  switch (request) {
    case ALARM_REQUEST_DISARM:
      return "disarming";
    case ALARM_REQUEST_TRIGGER:
      return "";
    default:
      return "arming";
  }
}

// Whether state is the panel working towards the request: an exit delay
// ("arming", or "pending" on older integrations) before armed, the entry delay
// ("pending") before triggered, "disarming" before disarmed.
inline bool alarm_request_in_progress(AlarmRequest request, const std::string &state) {
  switch (request) {
    case ALARM_REQUEST_DISARM:
      return state == "disarming";
    case ALARM_REQUEST_TRIGGER:
      return state == "pending";
    default:
      return state == "arming" || state == "pending";
  }
}

struct RemoteRequestProgress {
  std::string feedback;   // what the footer says now
  bool complete = false;  // nothing left to wait for
};

// Checked every second after the request goes out. A panel has
// start_window_ms to reach the requested state or start working towards it;
// an exit or entry delay may then run for up to max_wait_ms.
inline RemoteRequestProgress evaluate_alarm_request(AlarmRequest request, const std::string &state, uint32_t elapsed_ms,
                                                    uint32_t start_window_ms, uint32_t max_wait_ms) {
  RemoteRequestProgress progress;
  if (state == alarm_request_expected_state(request)) {
    progress.feedback = "SUCCESS";
    progress.complete = true;
  } else if (alarm_request_in_progress(request, state)) {
    progress.feedback = alarm_request_progress_feedback(request);
    progress.complete = elapsed_ms >= max_wait_ms;  // still counting down: stop watching, no verdict
  } else if (elapsed_ms >= start_window_ms) {
    progress.feedback = "FAILED";
    progress.complete = true;
  } else {
    progress.feedback = alarm_request_progress_feedback(request);
  }
  return progress;
}

// Locks. "open" (unlatched) counts as unlocked.
inline bool lock_state_unlocked(const std::string &state) { return state == "unlocked" || state == "open"; }

// Checked every second after lock.lock or lock.unlock goes out. start_state is
// the lock's state when the command went out, so a jam reported before it
// isn't taken for the answer. final is the last check.
inline RemoteRequestProgress evaluate_lock_request(bool locking, const std::string &state,
                                                   const std::string &start_state, bool final) {
  RemoteRequestProgress progress;
  if (locking ? state == "locked" : lock_state_unlocked(state)) {
    progress.feedback = locking ? "LOCKED" : (state == "open" ? "OPENED" : "UNLOCKED");
    progress.complete = true;
  } else if (state == "locking") {
    progress.feedback = "LOCKING...";
  } else if (state == "unlocking") {
    progress.feedback = "UNLOCKING...";
  } else if (state == "opening") {
    progress.feedback = "OPENING...";
  } else if (state == "jammed" && (start_state != "jammed" || final)) {
    progress.feedback = "JAMMED";
    progress.complete = true;
  } else if (final) {
    progress.feedback = locking ? "LOCK FAILED" : "UNLOCK FAILED";
    progress.complete = true;
  } else {
    progress.feedback = locking ? "LOCKING..." : "UNLOCKING...";
  }
  return progress;
}

// Covers. Home Assistant reports "open" for any cover that isn't fully
// closed, so a cover that reports its position is only open once that
// position says so. start_position is where it was when the command went out
// (NAN when it doesn't report one): a cover that ends part-way after moving
// stopped there, but one that never moved didn't take the command.
inline RemoteRequestProgress evaluate_cover_request(bool opening, const std::string &state, float position,
                                                    float start_position, bool final) {
  RemoteRequestProgress progress;
  const bool has_position = !std::isnan(position);
  const bool reached = opening ? (has_position ? position >= 99.0f : state == "open")
                               : (state == "closed" || (has_position && position <= 1.0f));
  if (reached) {
    progress.feedback = opening ? "OPENED" : "CLOSED";
    progress.complete = true;
  } else if (state == "opening") {
    progress.feedback = "OPENING...";
  } else if (state == "closing") {
    progress.feedback = "CLOSING...";
  } else if (has_position && !std::isnan(start_position) && std::fabs(position - start_position) >= 1.0f) {
    // Moved, and stopped part-way (or is still moving without saying so).
    char moved[16];
    snprintf(moved, sizeof(moved), "OPEN %d%%", static_cast<int>(std::lround(position)));
    progress.feedback = moved;
    progress.complete = final;
  } else if (final) {
    // A cover without state feedback reports "unknown" throughout: Home
    // Assistant took the command, and that is all the remote can know.
    progress.feedback = (state == "unknown" && !has_position) ? "SENT" : (opening ? "OPEN FAILED" : "CLOSE FAILED");
    progress.complete = true;
  } else {
    progress.feedback = opening ? "OPENING..." : "CLOSING...";
  }
  return progress;
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
