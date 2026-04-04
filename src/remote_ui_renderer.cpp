#include "remote_ui_renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"

namespace esphome {

static inline const std::string &render_string(const std::string *value) {
  static const std::string empty;
  return value != nullptr ? *value : empty;
}

static inline bool ui_recent_interaction(uint32_t now, uint32_t last_interaction, uint32_t duration_ms) {
  return last_interaction > 0 && (now - last_interaction) <= duration_ms;
}

static inline std::string ui_power_state_label(const std::string &raw) {
  if (raw == "on") return "ON";
  if (raw == "off") return "OFF";
  return "SYNCING";
}

static inline std::array<std::string, 3> split_notification_lines(std::string message) {
  std::array<std::string, 3> lines;
  for (auto &ch : message) {
    if (ch == '\n' || ch == '\r' || ch == '\t') {
      ch = ' ';
    }
  }
  if (message.empty()) {
    message = "NO NOTIFICATIONS";
  }

  bool truncated = message.size() > 66;
  if (truncated) {
    message = message.substr(0, 66);
  }

  lines[0] = message.substr(0, std::min<size_t>(22, message.size()));
  lines[1] = message.size() > 22 ? message.substr(22, std::min<size_t>(22, message.size() - 22)) : "";
  lines[2] = message.size() > 44 ? message.substr(44, std::min<size_t>(22, message.size() - 44)) : "";
  if (truncated && lines[2].size() >= 3) {
    lines[2].replace(lines[2].size() - 3, 3, "...");
  } else if (truncated) {
    lines[2] = "...";
  }
  return lines;
}

void render_remote_ui(
    display::Display *it, font::Font *tiny_font, font::Font *small_font, font::Font *medium_font,
    font::Font *symbols, font::Font *medium_symbols, const RemoteRenderContext &ctx) {
  const std::string &selected_item_name = render_string(ctx.selected_item_name);
  const std::string &selected_item_state = render_string(ctx.selected_item_state);
  const std::string &selected_humidifier_action = render_string(ctx.selected_humidifier_action);
  const std::string &selected_humidifier_mode = render_string(ctx.selected_humidifier_mode);
  const std::string &selected_climate_hvac_action = render_string(ctx.selected_climate_hvac_action);
  const std::string &selected_media_title = render_string(ctx.selected_media_title);
  const std::string &selected_media_artist = render_string(ctx.selected_media_artist);
  const std::string &selected_media_device_class = render_string(ctx.selected_media_device_class);
  const std::string &selected_media_source = render_string(ctx.selected_media_source);
  const std::string &last_media_power_feedback = render_string(ctx.last_media_power_feedback);
  const std::string &last_automation_feedback = render_string(ctx.last_automation_feedback);
  const std::string &last_alarm_feedback = render_string(ctx.last_alarm_feedback);
  const std::string &last_switch_feedback = render_string(ctx.last_switch_feedback);
  const std::string &last_lock_feedback = render_string(ctx.last_lock_feedback);
  const std::string &last_cover_feedback = render_string(ctx.last_cover_feedback);
  const std::string &selected_weather_condition = render_string(ctx.selected_weather_condition);
  char status_line[27];
  char footer_line[27];
  char detail_line[27];
  char label_primary[32];
  char label_secondary[32];
  char label_tertiary[32];
  const int bar_x = 14;
  const int bar_y = 45;
  const int bar_w = 100;
  const int bar_h = 8;
  status_line[0] = '\0';
  footer_line[0] = '\0';
  detail_line[0] = '\0';
  label_primary[0] = '\0';
  label_secondary[0] = '\0';
  label_tertiary[0] = '\0';

  bool show_brightness_bar =
      ui_recent_interaction(ctx.now, ctx.last_brightness_interaction, 3000) &&
      selected_item_state == "on" && ctx.mode == REMOTE_MODE_LIGHTS;
  bool show_fan_speed_bar =
      ui_recent_interaction(ctx.now, ctx.last_fan_speed_interaction, 3000) &&
      selected_item_state == "on" && ctx.mode == REMOTE_MODE_FANS;
  bool show_humidifier_target =
      ui_recent_interaction(ctx.now, ctx.last_humidifier_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_HUMIDIFIERS;
  bool show_humidifier_mode =
      ui_recent_interaction(ctx.now, ctx.last_humidifier_mode_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_HUMIDIFIERS;
  bool show_cover_position_bar =
      ui_recent_interaction(ctx.now, ctx.last_cover_position_interaction, 3000) &&
      ctx.mode == REMOTE_MODE_COVERS;
  bool show_switch_feedback =
      ui_recent_interaction(ctx.now, ctx.last_switch_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_SWITCHES;
  bool show_contrast_feedback = ui_recent_interaction(ctx.now, ctx.last_contrast_interaction, 3000);
  bool show_climate_target_focus =
      ui_recent_interaction(ctx.now, ctx.last_climate_target_focus_interaction, 5000) &&
      ctx.climate_target_focus != 0 && ctx.mode == REMOTE_MODE_CLIMATE;
  bool show_lock_feedback =
      ui_recent_interaction(ctx.now, ctx.last_lock_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_LOCKS;
  bool show_cover_feedback =
      ui_recent_interaction(ctx.now, ctx.last_cover_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_COVERS;
  bool show_media_feedback =
      ui_recent_interaction(ctx.now, ctx.last_media_volume_interaction, 3000) &&
      ctx.mode == REMOTE_MODE_MEDIA;
  bool show_media_source_feedback =
      ui_recent_interaction(ctx.now, ctx.last_media_source_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_MEDIA;
  bool show_media_power_feedback =
      ui_recent_interaction(ctx.now, ctx.last_media_power_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_MEDIA;
  bool show_automation_feedback =
      ui_recent_interaction(ctx.now, ctx.last_automation_interaction, 5000) &&
      ctx.mode == REMOTE_MODE_AUTOMATION;
  bool show_alarm_hold_prompt =
      ctx.mode == REMOTE_MODE_ALARMS &&
      ctx.settings_button_press_started_at > 0 &&
      ctx.settings_button_press_mode == static_cast<int>(ctx.mode) &&
      !ctx.settings_button_long_press_fired &&
      last_alarm_feedback == "HOLD TO TRIGGER";
  bool show_alarm_feedback =
      ctx.mode == REMOTE_MODE_ALARMS &&
      (ui_recent_interaction(ctx.now, ctx.last_alarm_interaction, 5000) || show_alarm_hold_prompt);

  auto has_dual_climate_target = [&]() {
    return !std::isnan(ctx.selected_climate_target_temp_low) &&
           !std::isnan(ctx.selected_climate_target_temp_high) &&
           (selected_item_state == "heat_cool" ||
            ctx.selected_climate_target_temp_low != ctx.selected_climate_target_temp_high);
  };
  auto draw_footer_dividers = [&]() {
    it->filled_rectangle(0, 52, 128, 1, display::COLOR_ON);
    it->filled_rectangle(38, 53, 1, 11, display::COLOR_ON);
    it->filled_rectangle(90, 53, 1, 11, display::COLOR_ON);
  };
  auto draw_contrast_footer = [&]() {
    snprintf(footer_line, sizeof(footer_line), "CONTRAST %d%%", ctx.contrast * 10);
    it->filled_rectangle(0, 52, 128, 1, display::COLOR_ON);
    it->print(64, 58, small_font, display::COLOR_ON, display::TextAlign::CENTER, footer_line);
  };
  auto draw_footer_chrome = [&](const char *left_icon, const char *right_icon) {
    draw_footer_dividers();
    it->printf(10, 50, medium_symbols, display::COLOR_ON, left_icon);
    it->printf(104, 50, medium_symbols, display::COLOR_ON, right_icon);
  };
  auto draw_default_footer = [&]() {
    draw_footer_chrome("\ueac3", "\ueac9");
    it->printf(58, 51, symbols, display::COLOR_ON, "\ue1ac");
  };
  auto draw_footer_text = [&](const char *text) {
    it->print(64, 58, small_font, display::COLOR_ON, display::TextAlign::CENTER, text);
  };
  auto draw_progress_bar = [&](int percent) {
    int fill = (bar_w - 2) * percent / 100;
    it->rectangle(bar_x, bar_y, bar_w, bar_h, display::COLOR_ON);
    if (fill > 0) {
      it->filled_rectangle(bar_x + 1, bar_y + 1, fill, bar_h - 2, display::COLOR_ON);
    }
  };
  auto draw_progress_footer = [&](int percent, const char *label) {
    draw_progress_bar(percent);
    snprintf(footer_line, sizeof(footer_line), "%s %d%%", label, percent);
    draw_footer_text(footer_line);
  };
  auto write_state_label = [&](const std::string &raw, char *buffer, size_t buffer_size, const char *fallback = "SYNCING") {
    remote_state_label_to_buffer(raw, buffer, buffer_size, fallback);
  };
  auto draw_centered_state = [&](const char *text, int y, bool compact = false) {
    if (compact) {
      it->print(64, y, small_font, display::COLOR_ON, display::TextAlign::CENTER, text);
    } else {
      it->print(64, y, medium_font, display::COLOR_ON, display::TextAlign::CENTER, text);
    }
  };
  auto draw_detail_text = [&](const char *text) {
    if (text != nullptr && text[0] != '\0') {
      it->print(64, 45, tiny_font, display::COLOR_ON, display::TextAlign::CENTER, text);
    }
  };
  auto draw_standard_footer = [&]() {
    if (show_contrast_feedback) {
      draw_contrast_footer();
    } else {
      draw_default_footer();
    }
  };
  auto draw_mode_footer = [&](const char *left_icon, const char *right_icon, const char *text = nullptr) {
    if (show_contrast_feedback) {
      draw_contrast_footer();
    } else {
      draw_footer_chrome(left_icon, right_icon);
      if (text != nullptr && text[0] != '\0') {
        draw_footer_text(text);
      } else {
        it->printf(58, 51, symbols, display::COLOR_ON, "\ue1ac");
      }
    }
  };
  auto draw_power_mode = [&](int percent_value, bool show_progress, const char *progress_label, bool draw_center_divider) {
    draw_centered_state(ui_power_state_label(selected_item_state).c_str(), 35);
    if (selected_item_state == "on" && !show_progress) {
      snprintf(detail_line, sizeof(detail_line), "%d%%", percent_value);
      draw_detail_text(detail_line);
    }
    if (show_progress) {
      draw_progress_footer(percent_value, progress_label);
    } else if (show_contrast_feedback) {
      draw_contrast_footer();
    } else {
      draw_default_footer();
      if (draw_center_divider) {
        it->filled_rectangle(90, 54, 1, 11, display::COLOR_ON);
      }
    }
  };
  auto draw_feedback_state_mode = [&](const char *state_text, const char *feedback_text, bool compact = false) {
    draw_centered_state(state_text, 33, compact);
    if (feedback_text != nullptr && feedback_text[0] != '\0') {
      draw_detail_text(feedback_text);
    }
    draw_standard_footer();
  };

  it->clear();
  const char *header_icon = mode_icon(ctx.mode);
  it->print(10, -3, symbols, display::COLOR_ON, display::TextAlign::LEFT, header_icon);
  it->printf(64, 4, small_font, display::COLOR_ON, display::TextAlign::CENTER, mode_title(ctx.mode));
  it->print(118, -3, symbols, display::COLOR_ON, display::TextAlign::RIGHT, header_icon);

  it->filled_rectangle(0, 12, 128, 1, display::COLOR_ON);
  if (ctx.mode != REMOTE_MODE_NOTIFICATIONS) {
    it->printf(64, 20, medium_font, display::COLOR_ON, display::TextAlign::CENTER, selected_item_name.c_str());
  }

  switch (ctx.mode) {
    case REMOTE_MODE_LIGHTS:
      draw_power_mode(ctx.selected_brightness_pct, show_brightness_bar, "BRIGHTNESS", true);
      break;

    case REMOTE_MODE_FANS:
      draw_power_mode(ctx.selected_fan_speed_pct, show_fan_speed_bar, "SPEED", true);
      break;

    case REMOTE_MODE_SWITCHES: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      if (show_switch_feedback) {
        write_state_label(last_switch_feedback, label_secondary, sizeof(label_secondary), "");
      }
      draw_feedback_state_mode(label_primary, show_switch_feedback ? label_secondary : "", false);
      break;
    }

    case REMOTE_MODE_CLIMATE: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      write_state_label(selected_climate_hvac_action, label_secondary, sizeof(label_secondary), "");
      bool dual_target = has_dual_climate_target();
      if (show_climate_target_focus) {
        if (!std::isnan(ctx.climate_target_focus_value)) {
          if (!dual_target) {
            snprintf(status_line, sizeof(status_line), "TARGET: %.0f°%s", ctx.climate_target_focus_value, ctx.temperature_unit);
          } else if (ctx.climate_target_focus == 2) {
            snprintf(status_line, sizeof(status_line), "HIGH: %.0f°%s", ctx.climate_target_focus_value, ctx.temperature_unit);
          } else {
            snprintf(status_line, sizeof(status_line), "LOW: %.0f°%s", ctx.climate_target_focus_value, ctx.temperature_unit);
          }
        }
      } else if (!std::isnan(ctx.selected_climate_current_temp)) {
        snprintf(status_line, sizeof(status_line), "%.0f°%s", ctx.selected_climate_current_temp, ctx.temperature_unit);
      } else {
        snprintf(status_line, sizeof(status_line), "SYNCING");
      }
      draw_centered_state(status_line, 35);

      if (!show_climate_target_focus && dual_target && strcmp(label_secondary, "UNKNOWN") != 0) {
        snprintf(detail_line, sizeof(detail_line), "LOW: %.0f°%s   HIGH: %.0f°%s",
                 ctx.selected_climate_target_temp_low, ctx.temperature_unit,
                 ctx.selected_climate_target_temp_high, ctx.temperature_unit);
        draw_detail_text(detail_line);
      } else if (!show_climate_target_focus && !std::isnan(ctx.selected_climate_target_temp) && strcmp(label_secondary, "UNKNOWN") != 0) {
        snprintf(detail_line, sizeof(detail_line), "TARGET: %.0f°%s", ctx.selected_climate_target_temp, ctx.temperature_unit);
        draw_detail_text(detail_line);
      }

      draw_mode_footer(
          "\ueac3", "\ueac9",
          (strcmp(label_primary, "SYNCING") != 0 && strcmp(label_secondary, "UNKNOWN") != 0) ? label_secondary : "");
      break;
    }

    case REMOTE_MODE_HUMIDIFIERS: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      write_state_label(selected_humidifier_action, label_secondary, sizeof(label_secondary), "");
      write_state_label(selected_humidifier_mode, label_tertiary, sizeof(label_tertiary), "");

      if (show_humidifier_target && !std::isnan(ctx.selected_humidifier_target_humidity)) {
        snprintf(status_line, sizeof(status_line), "TARGET: %.0f%%", ctx.selected_humidifier_target_humidity);
      } else if (!std::isnan(ctx.selected_humidifier_current_humidity)) {
        snprintf(status_line, sizeof(status_line), "%.0f%%", ctx.selected_humidifier_current_humidity);
      } else {
        snprintf(status_line, sizeof(status_line), "SYNCING");
      }
      draw_centered_state(status_line, 35);

      if (show_humidifier_mode && label_tertiary[0] != '\0') {
        draw_detail_text(label_tertiary);
      } else if (!show_humidifier_target && !std::isnan(ctx.selected_humidifier_target_humidity)) {
        snprintf(detail_line, sizeof(detail_line), "TARGET: %.0f%%", ctx.selected_humidifier_target_humidity);
        draw_detail_text(detail_line);
      }

      const char *footer_status = "";
      if (strcmp(label_secondary, "UNKNOWN") != 0 && label_secondary[0] != '\0') {
        footer_status = label_secondary;
      } else if (strcmp(label_primary, "SYNCING") != 0) {
        footer_status = label_primary;
      }
      draw_mode_footer("\ueac3", "\ueac9", footer_status);
      break;
    }

    case REMOTE_MODE_LOCKS: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      if (show_lock_feedback) {
        write_state_label(last_lock_feedback, label_secondary, sizeof(label_secondary), "");
      }
      draw_feedback_state_mode(label_primary, show_lock_feedback ? label_secondary : "", true);
      break;
    }

    case REMOTE_MODE_COVERS: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      write_state_label(last_cover_feedback, label_secondary, sizeof(label_secondary), "");
      draw_centered_state(label_primary, 33, true);
      if (show_cover_position_bar) {
        draw_progress_footer(ctx.selected_cover_position_pct, "POSITION");
      } else {
        const char *cover_detail = "";
        if (show_cover_feedback && label_secondary[0] != '\0') {
          cover_detail = label_secondary;
        } else if (strcmp(label_primary, "OPEN") == 0) {
          snprintf(detail_line, sizeof(detail_line), "%d%%", ctx.selected_cover_position_pct);
          cover_detail = detail_line;
        }
        draw_detail_text(cover_detail);
        draw_standard_footer();
      }
      break;
    }

    case REMOTE_MODE_MEDIA: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      bool is_tv = selected_media_device_class == "tv" || selected_media_device_class == "receiver";
      bool is_speaker = selected_media_device_class == "speaker";
      bool media_is_on = selected_item_state != "off" && selected_item_state != "unknown";
      if (is_tv) {
        snprintf(status_line, sizeof(status_line), "%s", media_is_on ? "ON" : "OFF");
        draw_centered_state(status_line, 33);
      } else if (!selected_media_title.empty()) {
        draw_centered_state(selected_media_title.c_str(), 33, true);
      }
      const char *media_detail = "";
      if (!is_tv && !show_media_feedback && show_media_power_feedback && !last_media_power_feedback.empty()) {
        media_detail = last_media_power_feedback.c_str();
      } else if (!show_media_feedback && show_media_source_feedback && !selected_media_source.empty()) {
        media_detail = selected_media_source.c_str();
      } else if (is_tv && media_is_on && !selected_media_source.empty()) {
        media_detail = selected_media_source.c_str();
      } else if (!show_media_feedback && !selected_media_artist.empty()) {
        media_detail = selected_media_artist.c_str();
      }
      draw_detail_text(media_detail);

      if (show_media_feedback) {
        draw_progress_footer(ctx.selected_media_volume_pct, "VOLUME");
      } else if (show_contrast_feedback) {
        draw_contrast_footer();
      } else {
        draw_footer_chrome(is_tv ? "\uead0" : "\ue045", is_tv ? "\ueacf" : "\ue044");
        if (strcmp(label_primary, "PAUSED") == 0) {
          it->printf(64, 58, tiny_font, display::COLOR_ON, display::TextAlign::CENTER, "PAUSED");
        } else if (strcmp(label_primary, "PLAYING") == 0) {
          it->printf(64, 58, tiny_font, display::COLOR_ON, display::TextAlign::CENTER, "PLAYING");
        } else if (strcmp(label_primary, "IDLE") == 0) {
          it->printf(64, 58, tiny_font, display::COLOR_ON, display::TextAlign::CENTER, "IDLE");
        } else {
          it->printf(58, 51, symbols, display::COLOR_ON, is_speaker ? "\ue047" : "\ue8ac");
        }
      }
      break;
    }

    case REMOTE_MODE_SENSORS: {
      std::string sensor_value = selected_item_state;
      if (sensor_value == "unknown" || sensor_value.empty()) {
        sensor_value = "SYNCING";
      } else if (sensor_value == "on") {
        sensor_value = "ON";
      } else if (sensor_value == "off") {
        sensor_value = "OFF";
      }
      draw_centered_state(sensor_value.c_str(), 33, sensor_value.size() > 12);
      draw_standard_footer();
      break;
    }

    case REMOTE_MODE_AUTOMATION: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary), "READY");
      write_state_label(last_automation_feedback, label_secondary, sizeof(label_secondary), "");
      const char *automation_type = automation_kind_label(ctx.automation_index);
      snprintf(status_line, sizeof(status_line), "%s",
               (show_automation_feedback && label_secondary[0] != '\0') ? label_secondary : label_primary);
      it->print(64, 33, small_font, display::COLOR_ON, display::TextAlign::CENTER, automation_type);
      draw_detail_text(status_line);
      draw_standard_footer();
      break;
    }

    case REMOTE_MODE_ALARMS: {
      write_state_label(selected_item_state, label_primary, sizeof(label_primary));
      if (show_alarm_feedback) {
        write_state_label(last_alarm_feedback, label_secondary, sizeof(label_secondary), "");
      }
      draw_feedback_state_mode(label_primary, show_alarm_feedback ? label_secondary : "", true);
      break;
    }

    case REMOTE_MODE_NOTIFICATIONS: {
      bool show_dismiss_feedback =
          ctx.last_notification_dismiss_interaction > 0 &&
          (ctx.now - ctx.last_notification_dismiss_interaction) <= 3000;
      auto lines = show_dismiss_feedback ? std::array<std::string, 3>{"", "DISMISSED", ""}
                                         : split_notification_lines(selected_item_state);
      for (int i = 0; i < 3; i++) {
        if (!lines[i].empty()) {
          it->print(
              64, i == 0 ? 20 : (i == 1 ? 33 : 45), tiny_font, display::COLOR_ON, display::TextAlign::CENTER,
              lines[i].c_str());
        }
      }
      draw_standard_footer();
      break;
    }

    case REMOTE_MODE_WEATHER: {
      write_state_label(selected_weather_condition, label_primary, sizeof(label_primary));
      if (!std::isnan(ctx.selected_weather_temperature)) {
        snprintf(status_line, sizeof(status_line), "%.0f°%s", ctx.selected_weather_temperature, ctx.temperature_unit);
      } else {
        snprintf(status_line, sizeof(status_line), "SYNCING");
      }
      draw_centered_state(status_line, 33);
      if (strcmp(label_primary, "SYNCING") != 0 && label_primary[0] != '\0') {
        it->print(64, 45, small_font, display::COLOR_ON, display::TextAlign::CENTER, label_primary);
      }
      const char *footer_status = "";
      if (strcmp(label_primary, "SYNCING") != 0 && !std::isnan(ctx.selected_weather_humidity)) {
        snprintf(footer_line, sizeof(footer_line), "%.0f%%", ctx.selected_weather_humidity);
        footer_status = footer_line;
      }
      draw_mode_footer("\ueac3", "\ueac9", footer_status);
      break;
    }

    case REMOTE_MODE_INFO:
    default:
      if (ctx.info_index == 0) {
        it->print(64, 33, medium_font, display::COLOR_ON, display::TextAlign::CENTER, ctx.info_primary_text.c_str());
        it->print(64, 45, small_font, display::COLOR_ON, display::TextAlign::CENTER, ctx.info_secondary_text.c_str());
      } else if (ctx.info_index == 1) {
        it->print(64, 33, small_font, display::COLOR_ON, display::TextAlign::CENTER, ctx.info_primary_text.c_str());
        it->print(64, 45, tiny_font, display::COLOR_ON, display::TextAlign::CENTER, ctx.info_secondary_text.c_str());
      } else {
        it->print(64, 33, medium_font, display::COLOR_ON, display::TextAlign::CENTER, ctx.info_primary_text.c_str());
        it->print(64, 45, tiny_font, display::COLOR_ON, display::TextAlign::CENTER, ctx.info_secondary_text.c_str());
      }
      if (show_contrast_feedback) {
        draw_contrast_footer();
      } else {
        draw_default_footer();
      }
      break;
  }

}

}  // namespace esphome
