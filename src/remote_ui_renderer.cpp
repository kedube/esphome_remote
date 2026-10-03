#include "remote_ui_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <strings.h>

#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"

namespace esphome {

namespace {

using display::Display;
using display::TextAlign;

const Color ON = display::COLOR_ON;
const Color OFF = display::COLOR_OFF;

// Screen layout (128x64):
//   0-8    header: list chip, clock, position dots, battery
//   12-25  entity name
//   26-50  hero: badge or graphic on the left, value to its right
//   53-63  footer: the selected control, button hints, hold progress or a toast
constexpr int SCREEN_W = 128;
constexpr int HEADER_H = 9;
constexpr int NAME_BASELINE = 22;
constexpr int HERO_CX = 13;
constexpr int HERO_CY = 38;
constexpr int HERO_R = 12;
constexpr int HERO_X = 31;            // left edge of the hero value column
constexpr int HERO_BASELINE = 48;     // large-font baseline
constexpr int HERO_WORD_BASELINE = 43;  // title-font baseline for state words
constexpr int FOOTER_Y = 53;
constexpr int FOOTER_H = 11;

constexpr uint32_t FEEDBACK_MS = 5000;
constexpr uint32_t TOAST_MS = 3000;

// Material Symbols Rounded codepoints. Every glyph used here must also be listed
// for the hero font in remote_fonts.yaml.
namespace icon {
const char *const LIGHTBULB = "\ue90f";
const char *const POWER = "\uf8c7";
const char *const THERMOSTAT = "\uf076";
const char *const HEAT = "\uf16a";
const char *const COOL = "\uf166";
const char *const HEAT_COOL = "\uf16b";
const char *const AUTO = "\uf077";
const char *const DRY = "\ue798";
const char *const FAN = "\uf168";
const char *const WATER_HEATER = "\ue284";
const char *const HUMIDITY = "\uf87e";
const char *const LOCK = "\ue899";
const char *const LOCK_OPEN = "\ue898";
const char *const SPEAKER = "\ue32d";
const char *const TV = "\ue63b";
const char *const PLAY = "\ue037";
const char *const PAUSE = "\ue034";
const char *const SENSOR = "\ue51e";
const char *const AUTOMATION = "\uf06c";
const char *const SCRIPT = "\ue86f";
const char *const SCENE = "\ue40a";
const char *const SHIELD = "\ue9e0";
const char *const SHIELD_ARMED = "\uf013";
const char *const SHIELD_ALERT = "\uf014";
const char *const ALL_CLEAR = "\ue2e6";
const char *const SUNNY = "\ue81a";
const char *const CLEAR_NIGHT = "\uf159";
const char *const PARTLY_DAY = "\uf172";
const char *const PARTLY_NIGHT = "\uf174";
const char *const CLOUD = "\uf15c";
const char *const RAIN = "\uf176";
const char *const STORM = "\uebdb";
const char *const SNOW = "\ue2cd";
const char *const FOG = "\ue818";
const char *const WIND = "\uefd8";
const char *const HAIL = "\uf67f";
const char *const CLOCK = "\uefd6";
const char *const WIFI = "\ue63e";
const char *const WIFI_OFF = "\ue648";
const char *const LAN = "\ueb2f";
const char *const DEVICE = "\ue30d";
const char *const BATTERY = "\ue1a5";
const char *const BATTERY_ALERT = "\ue19c";
const char *const INFO = "\ue88e";
const char *const RESTART = "\uf053";
const char *const CLOUD_OFF = "\ue2c1";
const char *const HOME = "\ue9b2";
const char *const ALERT = "\ue000";  // error: a jammed lock
const char *const VALVE = "\ue224";
const char *const PERSON = "\ue7fd";
const char *const PERSON_AWAY = "\uf150";  // location_away
const char *const TOUCH = "\ue913";        // touch_app: a button
const char *const TUNE = "\ue429";         // a number
const char *const LIST = "\ue896";         // a select
const char *const VACUUM = "\uefc5";
const char *const MOWER = "\uf205";        // grass
const char *const TIMER = "\ue425";
const char *const TIMER_PAUSE = "\uf4bb";
const char *const VOLUME_OFF = "\ue04f";
}  // namespace icon

const std::string &str(const std::string *value) {
  static const std::string empty;
  return value != nullptr ? *value : empty;
}

bool recent(uint32_t now, uint32_t at, uint32_t window_ms) { return at != 0 && now - at <= window_ms; }

bool truthy(const std::string &value) { return value == "on" || value == "true" || value == "True"; }

// ---- Text ------------------------------------------------------------------

// ESPHome's font code reads a whole UTF-8 character once it sees the first
// byte, so text ending in a character that snprintf or a fixed buffer cut short
// would make it read past the terminator. Returns text without that tail.
const char *whole_chars(const char *text, char *buf, size_t size) {
  size_t len = strlen(text);
  size_t keep = utf8_complete_length(text, len);
  if (keep == len) {
    return text;
  }
  keep = utf8_complete_length(text, std::min(keep, size - 1));
  memcpy(buf, text, keep);
  buf[keep] = '\0';
  return buf;
}

int text_width(font::Font *font, const char *text) {
  if (font == nullptr || text == nullptr || text[0] == '\0') {
    return 0;
  }
  char buf[64];
  int width, x_offset, baseline, height;
  font->measure(whole_chars(text, buf, sizeof(buf)), &width, &x_offset, &baseline, &height);
  return width;
}

// Returns text unchanged when it fits, otherwise a copy in buf cut at a UTF-8
// boundary with an ellipsis appended.
const char *fit_text(font::Font *font, const char *text, int max_width, char *buf, size_t buf_size) {
  static const char ELLIPSIS[] = "\u2026";
  if (text_width(font, text) <= max_width || buf_size <= sizeof(ELLIPSIS)) {
    return text;
  }
  size_t len = strnlen(text, buf_size - sizeof(ELLIPSIS));
  while (len > 0) {
    len--;
    while (len > 0 && (static_cast<unsigned char>(text[len]) & 0xC0) == 0x80) {
      len--;
    }
    size_t cut = len;
    while (cut > 0 && text[cut - 1] == ' ') {
      cut--;
    }
    memcpy(buf, text, cut);
    memcpy(buf + cut, ELLIPSIS, sizeof(ELLIPSIS));
    if (text_width(font, buf) <= max_width) {
      return buf;
    }
  }
  memcpy(buf, ELLIPSIS, sizeof(ELLIPSIS));
  return buf;
}

void text(Display *it, font::Font *font, int x, int baseline, TextAlign align, const char *value, Color color = ON) {
  if (font != nullptr && value != nullptr && value[0] != '\0') {
    char buf[64];
    it->print(x, baseline, font, color, align, whole_chars(value, buf, sizeof(buf)));
  }
}

// Prints text shortened to max_width.
void text_fit(Display *it, font::Font *font, int x, int baseline, TextAlign align, const char *value, int max_width,
              Color color = ON) {
  char buf[64];
  text(it, font, x, baseline, align, fit_text(font, value, max_width, buf, sizeof(buf)), color);
}

// Upper-cases an HA state ("heat_cool" -> "HEAT COOL") into buf.
const char *label(const std::string &raw, char *buf, size_t size, const char *fallback = "") {
  remote_state_label_to_buffer(raw, buf, size, fallback);
  return buf;
}

// Decodes the character at s. Each continuation byte is checked before the
// next is read, so a character cut short stops at the terminator.
uint32_t utf8_codepoint(const char *s) {
  const auto *p = reinterpret_cast<const uint8_t *>(s);
  auto cont = [p](int i) { return (p[i] & 0xC0) == 0x80; };
  if (p[0] < 0x80) return p[0];
  if ((p[0] & 0xE0) == 0xC0 && cont(1)) return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
  if ((p[0] & 0xF0) == 0xE0 && cont(1) && cont(2)) return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
  if ((p[0] & 0xF8) == 0xF0 && cont(1) && cont(2) && cont(3)) {
    return ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
  }
  return 0xFFFD;  // replacement character: no font here has a glyph for it
}

// Draws one icon glyph centred on its ink, not its em box, so every icon sits
// in the middle of a badge regardless of the font's metrics.
void draw_icon(Display *it, font::Font *font, int cx, int cy, const char *glyph, Color color = ON) {
  if (font == nullptr || glyph == nullptr) {
    return;
  }
  const font::Glyph *g = font->find_glyph(utf8_codepoint(glyph));
  if (g == nullptr) {
    return;
  }
  font->print(cx - g->offset_x - g->width / 2, cy - g->offset_y - g->height / 2, it, color, glyph, OFF);
}

// Prints text that inverts where it crosses the filled part of a bar ending at
// split_x, so the label stays readable at any fill level.
void text_over_fill(Display *it, font::Font *font, int x, int baseline, TextAlign align, const char *value, int area_x,
                    int area_y, int area_w, int area_h, int split_x) {
  if (split_x > area_x) {
    it->start_clipping(area_x, area_y, split_x, area_y + area_h);
    text(it, font, x, baseline, align, value, OFF);
    it->end_clipping();
  }
  if (split_x < area_x + area_w) {
    it->start_clipping(split_x, area_y, area_x + area_w, area_y + area_h);
    text(it, font, x, baseline, align, value, ON);
    it->end_clipping();
  }
}

// ---- Shapes ------------------------------------------------------------------

void fill_round_rect(Display *it, int x, int y, int w, int h, Color color = ON) {
  it->filled_rectangle(x + 1, y, w - 2, h, color);
  it->vertical_line(x, y + 1, h - 2, color);
  it->vertical_line(x + w - 1, y + 1, h - 2, color);
}

void draw_round_rect(Display *it, int x, int y, int w, int h, Color color = ON) {
  it->horizontal_line(x + 1, y, w - 2, color);
  it->horizontal_line(x + 1, y + h - 1, w - 2, color);
  it->vertical_line(x, y + 1, h - 2, color);
  it->vertical_line(x + w - 1, y + 1, h - 2, color);
}

// A circle is always an odd number of pixels across, so for an even h each cap
// is two circles a pixel apart. With w == h this is a disc of any size.
void fill_pill(Display *it, int x, int y, int w, int h, Color color = ON) {
  int r = (h - 1) / 2;
  for (int cy : {y + r, y + h - 1 - r}) {
    it->filled_circle(x + r, cy, r, color);
    it->filled_circle(x + w - 1 - r, cy, r, color);
  }
  it->filled_rectangle(x + r, y, w - 2 * r, h, color);
}

void draw_pill(Display *it, int x, int y, int w, int h) {
  fill_pill(it, x, y, w, h, ON);
  fill_pill(it, x + 1, y + 1, w - 2, h - 2, OFF);
}

// Small left/right arrowheads used for "+/- cycles this" hints.
void draw_chevron(Display *it, int x, int cy, bool right, Color color = ON) {
  for (int i = 0; i < 4; i++) {
    int dx = right ? i : 3 - i;
    it->vertical_line(x + dx, cy - 3 + i, 7 - 2 * i, color);
  }
}

void draw_minus(Display *it, int cx, int cy, Color color = ON) { it->horizontal_line(cx - 2, cy, 5, color); }

void draw_plus(Display *it, int cx, int cy, Color color = ON) {
  it->horizontal_line(cx - 2, cy, 5, color);
  it->vertical_line(cx, cy - 2, 5, color);
}

// Icons of the two physical action buttons, for the footer hints.
void draw_square_button(Display *it, int x, int cy) { it->rectangle(x, cy - 3, 7, 7, ON); }

void draw_circle_button(Display *it, int cx, int cy) { it->circle(cx, cy, 3, ON); }

// A lit badge means "on/active": the icon is cut out of a filled circle. Off is
// a thin ring around the outline icon.
void draw_badge(Display *it, const RemoteUiFonts &f, const char *glyph, bool lit, int cx = HERO_CX,
                int cy = HERO_CY, int r = HERO_R) {
  if (lit) {
    it->filled_circle(cx, cy, r, ON);
    draw_icon(it, f.hero, cx, cy, glyph, OFF);
  } else {
    it->circle(cx, cy, r, ON);
    draw_icon(it, f.hero, cx, cy, glyph, ON);
  }
}

// Inverted label. Returns its width.
int draw_chip(Display *it, font::Font *font, int x, int y, int h, const char *value, bool filled = true) {
  int w = text_width(font, value) + 6;
  int baseline = y + (h + 7) / 2;
  if (filled) {
    fill_round_rect(it, x, y, w, h, ON);
    text(it, font, x + 3, baseline, TextAlign::BASELINE_LEFT, value, OFF);
  } else {
    draw_round_rect(it, x, y, w, h, ON);
    text(it, font, x + 3, baseline, TextAlign::BASELINE_LEFT, value, ON);
  }
  return w;
}

void draw_toggle(Display *it, int x, int y, int w, int h, bool on) {
  int knob = h - 4;
  if (on) {
    fill_pill(it, x, y, w, h, ON);
    fill_pill(it, x + w - 2 - knob, y + 2, knob, knob, OFF);
  } else {
    draw_pill(it, x, y, w, h);
    fill_pill(it, x + 2, y + 2, knob, knob, ON);
  }
}

// Horizontal meter with its label printed across it.
void draw_meter(Display *it, font::Font *font, int x, int y, int w, int h, int percent, const char *value) {
  percent = std::max(0, std::min(100, percent));
  draw_round_rect(it, x, y, w, h, ON);
  int fill = (w - 2) * percent / 100;
  if (fill > 0) {
    it->filled_rectangle(x + 1, y + 1, fill, h - 2, ON);
  }
  text_over_fill(it, font, x + w / 2, y + (h + 7) / 2, TextAlign::BASELINE_CENTER, value, x, y, w, h, x + 1 + fill);
}

void draw_battery(Display *it, int x, int y, int percent) {
  it->rectangle(x, y, 11, 7, ON);
  it->vertical_line(x + 11, y + 2, 3, ON);
  int fill = std::max(0, std::min(100, percent)) * 9 / 100;
  if (fill > 0) {
    it->filled_rectangle(x + 1, y + 1, fill, 5, ON);
  }
}

void draw_signal_bars(Display *it, int x, int bottom, int rssi) {
  int bars = rssi == 0 ? 0 : rssi >= -55 ? 4 : rssi >= -67 ? 3 : rssi >= -75 ? 2 : 1;
  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 2;
    if (i < bars) {
      it->filled_rectangle(x + i * 5, bottom - h + 1, 3, h, ON);
    } else {
      it->rectangle(x + i * 5, bottom - h + 1, 3, h, ON);
    }
  }
}

// ---- Header ----------------------------------------------------------------------

// Where draw_header last put the clock, for remote_ui_header_clock_box().
int last_clock_x = 0;
int last_clock_w = 0;

void draw_header(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  // Right: battery, then the clock. Fixed positions so they never jump around.
  int right = SCREEN_W;
  if (ctx.battery_monitoring_available) {
    right -= 12;
    draw_battery(it, right, 1, ctx.battery_percentage);
    right -= 4;
  }
  char clock[12] = "";
  if (ctx.clock_valid) {
    int hour = ctx.clock_hour % 12;
    snprintf(clock, sizeof(clock), "%d:%02d", hour == 0 ? 12 : hour, ctx.clock_minute);
  }
  int clock_w = text_width(f.tiny, clock);

  // Left: the list chip followed by the position within the list.
  char pos[24] = "";
  bool dots = ctx.item_count > 1 && ctx.item_count <= 8;
  if (ctx.item_count > 8) {
    snprintf(pos, sizeof(pos), "%d/%d", ctx.item_index + 1, ctx.item_count);
  }
  int pos_w = dots ? ctx.item_count * 4 + 3 : (pos[0] != '\0' ? text_width(f.tiny, pos) + 4 : 0);
  int x = 0;
  if (ctx.list_title != nullptr && ctx.list_title[0] != '\0') {
    // A long title takes the clock's space rather than being cut short.
    int room = right - pos_w - 2;
    if (clock_w > 0 && text_width(f.tiny, ctx.list_title) + 6 <= room - clock_w - 6) {
      room -= clock_w + 6;
    } else {
      clock_w = 0;
    }
    char buf[32];
    x = draw_chip(it, f.tiny, 0, 0, HEADER_H, fit_text(f.tiny, ctx.list_title, room - 6, buf, sizeof(buf)));
  }
  if (dots) {
    // Current position: a 3x3 block; the others single pixels.
    for (int i = 0; i < ctx.item_count; i++) {
      int cx = x + 4 + i * 4;
      if (i == ctx.item_index) {
        it->filled_rectangle(cx - 1, 3, 3, 3, ON);
      } else {
        it->draw_pixel_at(cx, 4, ON);
      }
    }
  } else if (pos[0] != '\0') {
    text(it, f.tiny, x + 4, 8, TextAlign::BASELINE_LEFT, pos);
  }
  if (clock_w > 0) {
    text(it, f.tiny, right, 8, TextAlign::BASELINE_RIGHT, clock);
  }
  last_clock_x = right - clock_w - 1;
  last_clock_w = clock_w > 0 ? clock_w + 2 : 0;
}

void draw_name(Display *it, const RemoteUiFonts &f, const char *name) {
  // Long names step down to the smaller name font, then to the small font,
  // before they are shortened.
  for (font::Font *font : {f.name, f.name_small}) {
    if (font != nullptr && text_width(font, name) <= SCREEN_W) {
      text(it, font, 0, NAME_BASELINE, TextAlign::BASELINE_LEFT, name);
      return;
    }
  }
  text_fit(it, f.small, 0, NAME_BASELINE - 1, TextAlign::BASELINE_LEFT, name, SCREEN_W);
}

// ---- Footer ------------------------------------------------------------------------

void footer_toast(Display *it, const RemoteUiFonts &f, const char *message) {
  fill_round_rect(it, 0, FOOTER_Y, SCREEN_W, FOOTER_H, ON);
  text_fit(it, f.tiny, SCREEN_W / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER, message, SCREEN_W - 6, OFF);
}

void footer_hold(Display *it, const RemoteUiFonts &f, const char *message, int progress) {
  draw_meter(it, f.tiny, 0, FOOTER_Y, SCREEN_W, FOOTER_H, progress, message);
}

// Chip naming the selected setting. Returns the x where the control may start.
int footer_chip(Display *it, const RemoteUiFonts &f, const char *name) {
  return draw_chip(it, f.tiny, 0, FOOTER_Y, FOOTER_H, name) + 3;
}

void footer_range(Display *it, const RemoteUiFonts &f, const char *name, int percent, const char *value) {
  int x = footer_chip(it, f, name);
  draw_meter(it, f.tiny, x, FOOTER_Y, SCREEN_W - x, FOOTER_H, percent, value);
}

// "- value +" for stepped values such as temperatures.
void footer_stepper(Display *it, const RemoteUiFonts &f, const char *name, const char *value) {
  int x = footer_chip(it, f, name);
  int cy = FOOTER_Y + FOOTER_H / 2;
  draw_minus(it, x + 3, cy);
  draw_plus(it, SCREEN_W - 4, cy);
  text_fit(it, f.tiny, (x + SCREEN_W) / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER, value, SCREEN_W - x - 16);
}

// "< value >" for lists the +/- buttons step through.
void footer_options(Display *it, const RemoteUiFonts &f, const char *name, const char *value) {
  int x = footer_chip(it, f, name);
  int cy = FOOTER_Y + FOOTER_H / 2;
  draw_chevron(it, x + 1, cy, false);
  draw_chevron(it, SCREEN_W - 5, cy, true);
  text_fit(it, f.tiny, (x + SCREEN_W) / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER,
           value != nullptr && value[0] != '\0' ? value : "-", SCREEN_W - x - 14);
}

void footer_toggle(Display *it, const RemoteUiFonts &f, const char *name, bool on) {
  int x = footer_chip(it, f, name);
  draw_toggle(it, SCREEN_W - 20, FOOTER_Y + 1, 20, 9, on);
  text(it, f.tiny, x + 2, FOOTER_Y + 9, TextAlign::BASELINE_LEFT, on ? "ON" : "OFF");
}

void footer_info(Display *it, const RemoteUiFonts &f, const char *name, const char *value) {
  int x = footer_chip(it, f, name);
  text_fit(it, f.tiny, (x + SCREEN_W) / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER,
           value != nullptr && value[0] != '\0' ? value : "-", SCREEN_W - x - 2);
}

// Labels for the square (left) and circle (right) action buttons. Protected
// actions say "HOLD" between them, or "HOLD TO ..." when there is one action.
void footer_hints(Display *it, const RemoteUiFonts &f, const char *square, const char *circle, bool hold) {
  int cy = FOOTER_Y + FOOTER_H / 2;
  int baseline = FOOTER_Y + 9;
  bool has_square = square != nullptr && square[0] != '\0';
  bool has_circle = circle != nullptr && circle[0] != '\0';
  char lone[32];
  if (hold && has_square != has_circle) {
    snprintf(lone, sizeof(lone), "HOLD TO %s", has_square ? square : circle);
    if (has_square) {
      square = lone;
    } else {
      circle = lone;
    }
    hold = false;
  }
  int left_end = 0;
  int right_start = SCREEN_W;
  if (has_square) {
    draw_square_button(it, 1, cy);
    text(it, f.tiny, 11, baseline, TextAlign::BASELINE_LEFT, square);
    left_end = 11 + text_width(f.tiny, square);
  }
  if (has_circle) {
    draw_circle_button(it, SCREEN_W - 5, cy);
    text(it, f.tiny, SCREEN_W - 11, baseline, TextAlign::BASELINE_RIGHT, circle);
    right_start = SCREEN_W - 11 - text_width(f.tiny, circle);
  }
  if (hold && right_start - left_end >= text_width(f.tiny, "HOLD") + 8) {
    text(it, f.tiny, (left_end + right_start) / 2, baseline, TextAlign::BASELINE_CENTER, "HOLD");
  }
}

// Segmented picker, used for the alarm arm mode.
void footer_segments(Display *it, const RemoteUiFonts &f, const char *const *names, int count, int selected) {
  int seg_w = SCREEN_W / count;
  for (int i = 0; i < count; i++) {
    int x = i * seg_w;
    int w = i == count - 1 ? SCREEN_W - x : seg_w - 1;
    if (i == selected) {
      fill_round_rect(it, x, FOOTER_Y, w, FOOTER_H, ON);
      text(it, f.tiny, x + w / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER, names[i], OFF);
    } else {
      draw_round_rect(it, x, FOOTER_Y, w, FOOTER_H, ON);
      text(it, f.tiny, x + w / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER, names[i], ON);
    }
  }
}

// ---- Hero helpers ------------------------------------------------------------------

// True when every character of text has a glyph in font.
bool has_glyphs(font::Font *font, const char *value) {
  for (const char *p = value; *p != '\0';) {
    if (font->find_glyph(utf8_codepoint(p)) == nullptr) {
      return false;
    }
    p++;
    while ((static_cast<unsigned char>(*p) & 0xC0) == 0x80) {
      p++;
    }
  }
  return true;
}

// Big value (digits, %, °, ON/OFF) to the right of the badge. Anything the
// large font cannot draw falls back to the title font. Returns the right edge.
int hero_value(Display *it, const RemoteUiFonts &f, const char *value, int x = HERO_X) {
  if (!has_glyphs(f.large, value)) {
    text_fit(it, f.title, x, HERO_WORD_BASELINE, TextAlign::BASELINE_LEFT, value, SCREEN_W - x);
    return x + std::min(text_width(f.title, value), SCREEN_W - x);
  }
  text(it, f.large, x, HERO_BASELINE, TextAlign::BASELINE_LEFT, value);
  return x + text_width(f.large, value);
}

void hero_word(Display *it, const RemoteUiFonts &f, const char *value, int x = HERO_X) {
  text_fit(it, f.title, x, HERO_WORD_BASELINE, TextAlign::BASELINE_LEFT, value, SCREEN_W - x);
}

// Small caption stacked in the hero's right-hand column.
void hero_caption(Display *it, const RemoteUiFonts &f, int baseline, const char *value, int min_x = HERO_X) {
  text_fit(it, f.tiny, SCREEN_W, baseline, TextAlign::BASELINE_RIGHT, value, SCREEN_W - min_x);
}

// Status chip in the hero's top-right corner: filled while the device is
// actively working, outlined while idle.
void hero_status_chip(Display *it, const RemoteUiFonts &f, const char *value, bool active, int min_x) {
  // Needs room for a few letters; a long label is shortened rather than dropped.
  if (value == nullptr || value[0] == '\0' || SCREEN_W - min_x < 24) {
    return;
  }
  char buf[24];
  const char *shown = fit_text(f.tiny, value, SCREEN_W - min_x - 6, buf, sizeof(buf));
  int w = text_width(f.tiny, shown) + 6;
  draw_chip(it, f.tiny, SCREEN_W - w, 26, 10, shown, active);
}

// "SET 71°" in the hero's bottom-right corner, clear of the value ending at
// min_x: the "SET" label goes first when space runs out.
void hero_setpoint(Display *it, const RemoteUiFonts &f, const char *value, int min_x) {
  int value_w = text_width(f.title, value);
  if (SCREEN_W - value_w < min_x) {
    return;
  }
  text(it, f.title, SCREEN_W, HERO_BASELINE, TextAlign::BASELINE_RIGHT, value);
  if (SCREEN_W - value_w - 2 - text_width(f.tiny, "SET") >= min_x) {
    text(it, f.tiny, SCREEN_W - value_w - 2, HERO_BASELINE, TextAlign::BASELINE_RIGHT, "SET");
  }
}

void format_temp(char *buf, size_t size, float value) {
  if (std::isnan(value)) {
    snprintf(buf, size, "--°");
  } else {
    snprintf(buf, size, "%.0f°", display_rounded(value, 0));
  }
}

// The large text for an item without a usable state: Home Assistant reports
// it unavailable or unknown, or hasn't sent it yet.
const char *missing_word(const std::string &state) { return missing_state_word(state); }

// A setpoint's number. Celsius thermostats step by 0.5, so a half degree stays
// ("21.5"); whole degrees print as "71".
const char *setpoint_number(char *buf, size_t size, float value) {
  bool whole = std::fabs(value - std::round(value)) < 0.05f;
  snprintf(buf, size, whole ? "%.0f" : "%.1f", display_rounded(value, whole ? 0 : 1));
  return buf;
}

void format_setpoint(char *buf, size_t size, float value) {
  if (std::isnan(value)) {
    snprintf(buf, size, "--°");
  } else {
    char number[12];
    snprintf(buf, size, "%s°", setpoint_number(number, sizeof(number), value));
  }
}

// Footer text for a reading: "--" until Home Assistant has sent it.
const char *or_dash(float reading, const char *formatted) { return std::isfinite(reading) ? formatted : "--"; }

// Meter fill for a reading: empty until Home Assistant has sent it.
int meter_percent(float reading) {
  return std::isfinite(reading) ? static_cast<int>(std::clamp(reading, 0.0f, 100.0f)) : 0;
}

// ---- Footer state shared by every mode ----------------------------------------------

// Hold progress, contrast, action feedback and status messages take the
// footer over from the mode's own control. Returns true when one was drawn.
bool draw_footer_overlay(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx, const char *feedback) {
  if (ctx.hold_progress >= 0 && ctx.hold_label != nullptr) {
    footer_hold(it, f, ctx.hold_label, ctx.hold_progress);
    return true;
  }
  if (recent(ctx.now, ctx.last_contrast_interaction, FEEDBACK_MS)) {
    char value[12];
    snprintf(value, sizeof(value), "%d%%", ctx.contrast * 10);
    footer_range(it, f, "CONTRAST", ctx.contrast * 10, value);
    return true;
  }
  if (feedback != nullptr && feedback[0] != '\0') {
    footer_toast(it, f, feedback);
    return true;
  }
  if (recent(ctx.now, ctx.toast_at, TOAST_MS) && !str(ctx.toast_text).empty()) {
    footer_toast(it, f, ctx.toast_text->c_str());
    return true;
  }
  return false;
}

// Returns a domain's feedback text when it is still fresh.
const char *fresh_feedback(const RemoteRenderContext &ctx, const std::string *feedback, uint32_t at, char *buf,
                           size_t size) {
  if (!recent(ctx.now, at, FEEDBACK_MS) || str(feedback).empty()) {
    return nullptr;
  }
  return label(*feedback, buf, size);
}

// The footer control for the selected setting.
void draw_setting_footer(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  auto option = static_cast<RemoteSettingOption>(ctx.selected_setting_option);
  const std::string &detail = str(ctx.selected_setting_detail);
  // The value just sent while it is fresh, otherwise what the entity reports.
  auto pending_or = [&detail](const std::string *reported) -> const std::string & {
    return detail.empty() ? str(reported) : detail;
  };
  char value[48];
  char upper[48];
  const char *t = ctx.temperature_unit;
  switch (option) {
    case REMOTE_SETTING_LIGHT_DIMMER:
      // 0 while on: just turned on, at a level Home Assistant hasn't reported yet.
      snprintf(value, sizeof(value), "%d%%", ctx.selected_brightness_pct);
      footer_range(it, f, "BRIGHTNESS", ctx.selected_brightness_pct, ctx.selected_brightness_pct > 0 ? value : "--");
      break;
    case REMOTE_SETTING_LIGHT_EFFECT:
      footer_options(it, f, "EFFECT", label(pending_or(ctx.light_effect), upper, sizeof(upper), "NONE"));
      break;
    case REMOTE_SETTING_LIGHT_WARMTH:
      // The meter fills as the light gets warmer, as Plus makes it.
      snprintf(value, sizeof(value), "%.0fK", ctx.light_color_temp_kelvin);
      footer_range(it, f, "WARMTH", warmth_percent(ctx.light_color_temp_kelvin, ctx.light_min_kelvin, ctx.light_max_kelvin),
                   or_dash(ctx.light_color_temp_kelvin, value));
      break;
    case REMOTE_SETTING_CLIMATE_LOW:
    case REMOTE_SETTING_CLIMATE_HIGH: {
      bool high = option == REMOTE_SETTING_CLIMATE_HIGH;
      float v = high ? ctx.selected_climate_target_temp_high : ctx.selected_climate_target_temp_low;
      if (ctx.climate_target_focus == (high ? 2 : 1) && !std::isnan(ctx.climate_target_focus_value)) {
        v = ctx.climate_target_focus_value;
      }
      char number[12];
      snprintf(value, sizeof(value), "%s°%s", setpoint_number(number, sizeof(number), v), t);
      footer_stepper(it, f, high ? "HIGH" : "LOW", std::isnan(v) ? "--" : value);
      break;
    }
    case REMOTE_SETTING_CLIMATE_TARGET: {
      float v = ctx.climate_target_focus != 0 && !std::isnan(ctx.climate_target_focus_value)
                    ? ctx.climate_target_focus_value
                    : ctx.selected_climate_target_temp;
      char number[12];
      snprintf(value, sizeof(value), "%s°%s", setpoint_number(number, sizeof(number), v), t);
      footer_stepper(it, f, "TARGET", std::isnan(v) ? "--" : value);
      break;
    }
    case REMOTE_SETTING_CLIMATE_FAN:
      footer_options(it, f, "FAN", label(str(ctx.selected_climate_fan_mode), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_CLIMATE_HUMIDITY:
      snprintf(value, sizeof(value), "%.0f%%", ctx.selected_climate_target_humidity);
      footer_range(it, f, "HUMIDITY", meter_percent(ctx.selected_climate_target_humidity),
                   or_dash(ctx.selected_climate_target_humidity, value));
      break;
    case REMOTE_SETTING_CLIMATE_PRESETS:
      footer_options(it, f, "PRESET", label(str(ctx.selected_climate_preset), upper, sizeof(upper), "NONE"));
      break;
    case REMOTE_SETTING_CLIMATE_ACTION:
      footer_info(it, f, "STATUS", label(str(ctx.selected_climate_hvac_action), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_CLIMATE_STATE:
    case REMOTE_SETTING_CLIMATE_HVAC_MODE:
      // A list to step through once the thermostat has said which modes it has.
      if (ctx.climate_mode_count > 1) {
        footer_options(it, f, "MODE", label(str(ctx.selected_item_state), upper, sizeof(upper), "SYNCING"));
      } else {
        footer_info(it, f, "MODE", label(str(ctx.selected_item_state), upper, sizeof(upper), "SYNCING"));
      }
      break;
    case REMOTE_SETTING_HUMIDIFIER_HUMIDITY:
      snprintf(value, sizeof(value), "%.0f%%", ctx.selected_humidifier_target_humidity);
      footer_range(it, f, "TARGET", meter_percent(ctx.selected_humidifier_target_humidity),
                   or_dash(ctx.selected_humidifier_target_humidity, value));
      break;
    case REMOTE_SETTING_HUMIDIFIER_MODE:
      footer_options(it, f, "MODE", label(str(ctx.selected_humidifier_mode), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_HUMIDIFIER_ACTION:
      footer_info(it, f, "STATUS", label(str(ctx.selected_humidifier_action), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_HUMIDIFIER_STATE:
      footer_info(it, f, "POWER", label(str(ctx.selected_item_state), upper, sizeof(upper), "SYNCING"));
      break;
    case REMOTE_SETTING_FAN_SPEED:
      // 0 while on: just turned on, at a speed Home Assistant hasn't reported yet.
      snprintf(value, sizeof(value), "%d%%", ctx.selected_fan_speed_pct);
      footer_range(it, f, "SPEED", ctx.selected_fan_speed_pct, ctx.selected_fan_speed_pct > 0 ? value : "--");
      break;
    case REMOTE_SETTING_FAN_PRESETS:
      footer_options(it, f, "PRESET", label(pending_or(ctx.fan_preset), upper, sizeof(upper), "NONE"));
      break;
    case REMOTE_SETTING_FAN_OSCILLATE:
      footer_toggle(it, f, "OSCILLATE", detail.empty() ? ctx.fan_oscillating == 1 : detail == "ON");
      break;
    case REMOTE_SETTING_FAN_DIRECTION:
      footer_options(it, f, "DIRECTION", label(pending_or(ctx.fan_direction), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_COVER_POSITION:
      snprintf(value, sizeof(value), "%d%%", ctx.selected_cover_position_pct);
      footer_range(it, f, "POSITION", ctx.selected_cover_position_pct, value);
      break;
    case REMOTE_SETTING_COVER_TILT:
      snprintf(value, sizeof(value), "%d%%", std::max(0, ctx.cover_tilt_pct));
      footer_range(it, f, "TILT", std::max(0, ctx.cover_tilt_pct), ctx.cover_tilt_pct < 0 ? "--" : value);
      break;
    case REMOTE_SETTING_MEDIA_SELECT:
      footer_options(it, f, "TRACK", "PREV  /  NEXT");
      break;
    case REMOTE_SETTING_MEDIA_CHANNEL:
      footer_options(it, f, "CHANNEL", "DOWN  /  UP");
      break;
    case REMOTE_SETTING_MEDIA_VOLUME:
      // -1: the player reports no volume (it is off, or hasn't synced).
      snprintf(value, sizeof(value), "%d%%", ctx.selected_media_volume_pct);
      footer_range(it, f, "VOLUME", std::max(ctx.selected_media_volume_pct, 0),
                   ctx.selected_media_volume_pct < 0 ? "--" : value);
      break;
    case REMOTE_SETTING_MEDIA_SHUFFLE:
      footer_toggle(it, f, "SHUFFLE", truthy(str(ctx.selected_media_shuffle)));
      break;
    case REMOTE_SETTING_MEDIA_SOURCE:
      footer_options(it, f, "SOURCE", label(str(ctx.selected_media_source), upper, sizeof(upper), "-"));
      break;
    case REMOTE_SETTING_MEDIA_REPEAT:
      footer_options(it, f, "REPEAT", label(str(ctx.selected_media_repeat), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_MEDIA_SOUND:
      footer_options(it, f, "SOUND", label(str(ctx.selected_media_sound_mode), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_MEDIA_STATE:
      footer_info(it, f, "STATE", label(str(ctx.selected_item_state), upper, sizeof(upper), "SYNCING"));
      break;
    case REMOTE_SETTING_MEDIA_MUTE:
      footer_toggle(it, f, "MUTE", detail.empty() ? ctx.media_muted == 1 : detail == "ON");
      break;
    case REMOTE_SETTING_INPUT_VALUE: {
      char number[24];
      format_ha_number(ctx.input_value, number, sizeof(number));
      const std::string &unit = str(ctx.input_unit);
      snprintf(value, sizeof(value), "%s%s%s", number, unit.empty() ? "" : " ", unit.c_str());
      int percent = 0;
      if (std::isfinite(ctx.input_value) && std::isfinite(ctx.input_min) && std::isfinite(ctx.input_max) &&
          ctx.input_max > ctx.input_min) {
        percent = meter_percent((ctx.input_value - ctx.input_min) * 100.0f / (ctx.input_max - ctx.input_min));
      }
      footer_range(it, f, "VALUE", percent, or_dash(ctx.input_value, value));
      break;
    }
    case REMOTE_SETTING_INPUT_OPTION:
      // A select's options are names, not state keys: kept as written, upper-cased.
      str_upper_to_buffer(str(ctx.selected_item_state), upper, sizeof(upper));
      footer_options(it, f, "OPTION", upper);
      break;
    case REMOTE_SETTING_VACUUM_FAN_SPEED:
      footer_options(it, f, "FAN", label(pending_or(ctx.vacuum_fan_speed), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_ALARM_STATE: {
      // Only the arm modes this panel supports.
      const char *names[ALARM_ARM_MODE_COUNT];
      int count = 0;
      int selected = 0;
      AlarmArmMode effective = alarm_effective_arm_mode(ctx.selected_alarm_arm_mode, ctx.alarm_supported_features);
      for (int i = 0; i < ALARM_ARM_MODE_COUNT; i++) {
        auto mode = static_cast<AlarmArmMode>(i);
        if (!alarm_arm_mode_supported(ctx.alarm_supported_features, mode)) {
          continue;
        }
        if (mode == effective) {
          selected = count;
        }
        names[count++] = alarm_arm_mode_short_label(mode);
      }
      if (count == 0) {
        footer_hints(it, f, "DISARM", nullptr, true);
      } else {
        footer_segments(it, f, names, count, selected);
      }
      break;
    }
    case REMOTE_SETTING_WATER_HEATER_TARGET: {
      char number[12];
      snprintf(value, sizeof(value), "%s°%s",
               setpoint_number(number, sizeof(number), ctx.selected_water_heater_target_temp), t);
      footer_stepper(it, f, "TARGET", std::isnan(ctx.selected_water_heater_target_temp) ? "--" : value);
      break;
    }
    case REMOTE_SETTING_WATER_HEATER_MODE:
      footer_options(it, f, "MODE", label(str(ctx.selected_water_heater_mode), upper, sizeof(upper)));
      break;
    case REMOTE_SETTING_WATER_HEATER_AWAY:
      footer_toggle(it, f, "AWAY", truthy(str(ctx.selected_water_heater_away)));
      break;
    default:
      break;
  }
}

// ---- Modes ---------------------------------------------------------------------------

void render_light(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool on = state == "on";
  draw_badge(it, f, icon::LIGHTBULB, on);
  int value_right = 0;
  if (ha_state_missing(state)) {
    hero_word(it, f, missing_word(state));
  } else if (!on) {
    hero_value(it, f, "OFF");
  } else if (ctx.selected_brightness_pct > 0) {
    char value[12];
    snprintf(value, sizeof(value), "%d%%", ctx.selected_brightness_pct);
    value_right = hero_value(it, f, value);
  } else {
    value_right = hero_value(it, f, "ON");
  }
  // The active effect, when it fits beside the value and is not already the
  // footer's subject.
  const std::string &effect = str(ctx.light_effect);
  bool no_effect = effect.empty() || strcasecmp(effect.c_str(), "none") == 0 || strcasecmp(effect.c_str(), "off") == 0;
  if (value_right > 0 && !no_effect && ctx.selected_setting_option != REMOTE_SETTING_LIGHT_EFFECT) {
    char buf[32];
    label(effect, buf, sizeof(buf));
    if (text_width(f.tiny, buf) <= SCREEN_W - value_right - 4) {
      hero_caption(it, f, 33, buf, value_right + 4);
    }
  }
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  if (on && ctx.selected_setting_option != REMOTE_SETTING_NONE) {
    draw_setting_footer(it, f, ctx);
  } else {
    footer_hints(it, f, "OFF", "ON", false);
  }
}

void render_switch(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool on = state == "on";
  // A big toggle stands in for the badge: switches have nothing else to show.
  if (ha_state_missing(state)) {
    draw_pill(it, 0, 29, 36, 19);
    hero_word(it, f, missing_word(state), 42);
  } else if (state == "turning_on" || state == "turning_off") {
    // Sent, not yet confirmed: the toggle shows where it is going.
    bool turning_on = state == "turning_on";
    draw_toggle(it, 0, 29, 36, 19, turning_on);
    hero_word(it, f, turning_on ? "TURNING ON" : "TURNING OFF", 42);
  } else {
    draw_toggle(it, 0, 29, 36, 19, on);
    hero_value(it, f, on ? "ON" : "OFF", 42);
  }
  char buf[32];
  if (draw_footer_overlay(it, f, ctx,
                          fresh_feedback(ctx, ctx.last_switch_feedback, ctx.last_switch_interaction, buf, sizeof(buf)))) {
    return;
  }
  footer_hints(it, f, "OFF", "ON", false);
}

const char *climate_mode_icon(const std::string &mode) {
  if (mode == "heat") return icon::HEAT;
  if (mode == "cool") return icon::COOL;
  if (mode == "heat_cool") return icon::HEAT_COOL;
  if (mode == "auto") return icon::AUTO;
  if (mode == "dry") return icon::DRY;
  if (mode == "fan_only") return icon::FAN;
  return icon::THERMOSTAT;
}

void render_climate(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &mode = str(ctx.selected_item_state);
  const std::string &action = str(ctx.selected_climate_hvac_action);
  bool off = mode == "off";
  draw_badge(it, f, climate_mode_icon(mode), !off && !ha_state_missing(mode));

  if (ha_state_missing(mode) && (mode == "unavailable" || std::isnan(ctx.selected_climate_current_temp))) {
    hero_word(it, f, missing_word(mode));
    if (!draw_footer_overlay(it, f, ctx, nullptr)) {
      draw_setting_footer(it, f, ctx);
    }
    return;
  }
  char current[12];
  format_temp(current, sizeof(current), ctx.selected_climate_current_temp);
  int value_right = hero_value(it, f, current);

  // The badge shows the mode; the chip what the system is doing right now.
  char status[16];
  label(action.empty() || ha_state_missing(action) ? mode : action, status, sizeof(status), "SYNCING");
  // The tracker upper-cases hvac_action ("IDLE"); compare without case.
  bool running = !action.empty() && strcasecmp(action.c_str(), "idle") != 0 &&
                 strcasecmp(action.c_str(), "off") != 0 && !ha_state_missing(action);
  hero_status_chip(it, f, status, running, value_right + 3);

  bool focus = recent(ctx.now, ctx.last_climate_target_focus_interaction, FEEDBACK_MS) && ctx.climate_target_focus != 0 &&
               !std::isnan(ctx.climate_target_focus_value);
  bool dual = remote_ui_has_dual_climate_target(mode, ctx.selected_climate_target_temp_low,
                                                ctx.selected_climate_target_temp_high);
  char target[24];
  if (dual) {
    float low = focus && ctx.climate_target_focus == 1 ? ctx.climate_target_focus_value : ctx.selected_climate_target_temp_low;
    float high = focus && ctx.climate_target_focus == 2 ? ctx.climate_target_focus_value : ctx.selected_climate_target_temp_high;
    char low_text[12];
    char high_text[12];
    snprintf(target, sizeof(target), "%s-%s°", setpoint_number(low_text, sizeof(low_text), low),
             setpoint_number(high_text, sizeof(high_text), high));
    // Half degrees can make a range too wide to sit beside the reading: drop
    // the degree sign, then round to whole degrees (half up, so 21.5-22.5
    // doesn't print as 22-22).
    if (text_width(f.title, target) > SCREEN_W - value_right - 4) {
      snprintf(target, sizeof(target), "%s-%s", low_text, high_text);
    }
    if (text_width(f.title, target) > SCREEN_W - value_right - 4) {
      snprintf(target, sizeof(target), "%.0f-%.0f°", display_rounded(std::floor(low + 0.5f), 0),
               display_rounded(std::floor(high + 0.5f), 0));
    }
  } else {
    float value = focus ? ctx.climate_target_focus_value : ctx.selected_climate_target_temp;
    format_setpoint(target, sizeof(target), value);
  }
  if (!off && (dual || !std::isnan(ctx.selected_climate_target_temp))) {
    hero_setpoint(it, f, target, value_right + 4);
  }

  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  draw_setting_footer(it, f, ctx);
}

void render_water_heater(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool on = !ha_state_missing(state) && state != "off";
  draw_badge(it, f, icon::WATER_HEATER, on);
  char target[12];
  format_setpoint(target, sizeof(target), ctx.selected_water_heater_target_temp);
  int value_right = HERO_X + 40;
  if (state == "unavailable") {
    hero_word(it, f, missing_word(state));
  } else {
    value_right = hero_value(it, f, target);
  }
  char mode[24];
  label(str(ctx.selected_water_heater_mode), mode, sizeof(mode), ha_state_missing(state) ? "" : on ? "ON" : "OFF");
  hero_status_chip(it, f, mode, on, value_right + 3);
  if (truthy(str(ctx.selected_water_heater_away))) {
    hero_caption(it, f, HERO_BASELINE, "AWAY", 90);
  }
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  if (ctx.selected_setting_option != REMOTE_SETTING_NONE) {
    draw_setting_footer(it, f, ctx);
  } else {
    footer_hints(it, f, "OFF", "ON", false);
  }
}

void render_humidifier(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool on = state == "on";
  draw_badge(it, f, icon::HUMIDITY, on);
  // The big value is the room's humidity; the target goes in the corner with
  // SET, so the two can't be mistaken for each other. A humidifier without a
  // sensor shows ON/OFF instead.
  char value[12];
  int value_right = HERO_X + 40;
  if (std::isnan(ctx.selected_humidifier_current_humidity) || state == "unavailable") {
    hero_word(it, f, ha_state_missing(state) ? missing_word(state) : (on ? "ON" : "OFF"));
  } else {
    snprintf(value, sizeof(value), "%.0f%%", ctx.selected_humidifier_current_humidity);
    value_right = hero_value(it, f, value);
  }
  if (!std::isnan(ctx.selected_humidifier_target_humidity)) {
    char target[12];
    snprintf(target, sizeof(target), "%.0f%%", ctx.selected_humidifier_target_humidity);
    hero_setpoint(it, f, target, value_right + 4);
  }
  // HA reports humidifying/drying/idle/off; "HUMIDIFYING" is too wide for the chip.
  const std::string &action = str(ctx.selected_humidifier_action);
  char status[16];
  if (action == "humidifying") {
    snprintf(status, sizeof(status), "ACTIVE");
  } else {
    label(action.empty() || ha_state_missing(action) ? state : action, status, sizeof(status));
  }
  hero_status_chip(it, f, status, action == "humidifying" || action == "drying", value_right + 3);
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  draw_setting_footer(it, f, ctx);
}

void render_fan(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool on = state == "on";
  draw_badge(it, f, icon::FAN, on);
  if (ha_state_missing(state)) {
    hero_word(it, f, missing_word(state));
  } else if (on && ctx.selected_fan_speed_pct > 0) {
    char value[12];
    snprintf(value, sizeof(value), "%d%%", ctx.selected_fan_speed_pct);
    hero_value(it, f, value);
  } else {
    hero_value(it, f, on ? "ON" : "OFF");
  }
  // Speed steps, tallest last, lit up to the current speed.
  int lit = on ? (ctx.selected_fan_speed_pct + 19) / 20 : 0;
  for (int i = 0; i < 5; i++) {
    int h = 4 + i * 3;
    int x = SCREEN_W - 25 + i * 5;
    if (i < lit) {
      it->filled_rectangle(x, HERO_BASELINE - h, 4, h, ON);
    } else {
      it->rectangle(x, HERO_BASELINE - h, 4, h, ON);
    }
  }
  if (on && ctx.fan_oscillating == 1) {
    hero_caption(it, f, 31, "OSC", 100);
  }
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  if (on && ctx.selected_setting_option != REMOTE_SETTING_NONE) {
    draw_setting_footer(it, f, ctx);
  } else {
    footer_hints(it, f, "OFF", "ON", false);
  }
}

// A window with the shade drawn down to the current position.
void draw_cover(Display *it, int position, bool known) {
  const int x = 2, y = 26, w = 22, h = 25;
  it->rectangle(x, y, w, h, ON);
  it->horizontal_line(x - 1, y, w + 2, ON);
  int inner = h - 2;
  int covered = known ? inner * (100 - std::max(0, std::min(100, position))) / 100 : inner / 2;
  for (int row = 0; row < covered; row += 2) {
    it->horizontal_line(x + 1, y + 1 + row, w - 2, ON);
  }
  if (covered > 0) {
    it->horizontal_line(x + 1, y + covered, w - 2, ON);
    it->filled_rectangle(x + w / 2 - 1, y + covered + 1, 3, 2, ON);  // pull handle
  }
}

void render_cover(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool known = ctx.cover_has_position;
  if (ctx.cover_is_valve) {
    // A valve is lit while any water or gas can flow.
    bool open = state == "open" || state == "opening" || (known && state != "closed" && ctx.selected_cover_position_pct > 0);
    draw_badge(it, f, icon::VALVE, open && !ha_state_missing(state));
  } else {
    int position = known ? ctx.selected_cover_position_pct : (state == "open" ? 100 : 0);
    draw_cover(it, position, known || state == "open" || state == "closed");
  }
  char word[16];
  label(state, word, sizeof(word), "SYNCING");
  if (known && !ha_state_missing(state)) {
    char value[12];
    snprintf(value, sizeof(value), "%d%%", ctx.selected_cover_position_pct);
    int value_right = hero_value(it, f, value);
    hero_caption(it, f, 33, word, value_right + 4);
  } else {
    hero_word(it, f, word);
  }
  char buf[32];
  if (draw_footer_overlay(it, f, ctx,
                          fresh_feedback(ctx, ctx.last_cover_feedback, ctx.last_cover_interaction, buf, sizeof(buf)))) {
    return;
  }
  // While it moves either button stops it.
  if (ctx.cover_stoppable) {
    footer_hints(it, f, "STOP", "STOP", false);
  } else if (ctx.selected_setting_option != REMOTE_SETTING_NONE) {
    draw_setting_footer(it, f, ctx);
  } else {
    footer_hints(it, f, "CLOSE", "OPEN", true);
  }
}

void render_lock(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool locked = state == "locked";
  bool jammed = state == "jammed";
  bool unlocked = lock_state_unlocked(state) || state == "unlocking" || state == "opening";
  draw_badge(it, f, jammed ? icon::ALERT : unlocked ? icon::LOCK_OPEN : icon::LOCK, locked || jammed);
  char word[16];
  hero_word(it, f, label(state, word, sizeof(word), "SYNCING"));
  char buf[32];
  if (draw_footer_overlay(it, f, ctx,
                          fresh_feedback(ctx, ctx.last_lock_feedback, ctx.last_lock_interaction, buf, sizeof(buf)))) {
    return;
  }
  footer_hints(it, f, "UNLOCK", "LOCK", true);
}

void render_media(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  const std::string &device_class = str(ctx.selected_media_device_class);
  bool is_tv = device_class == "tv" || device_class == "receiver";
  bool on = !ha_state_missing(state) && state != "off" && state != "standby";
  if (is_tv) {
    draw_badge(it, f, icon::TV, on);
    if (ha_state_missing(state)) {
      hero_word(it, f, missing_word(state));
    } else {
      hero_value(it, f, on ? "ON" : "OFF");
    }
    char source[32];
    if (on && !str(ctx.selected_media_source).empty()) {
      hero_caption(it, f, 33, label(*ctx.selected_media_source, source, sizeof(source)), 84);
    }
    if (on && ctx.media_muted == 1) {
      hero_caption(it, f, HERO_BASELINE, "MUTED", 84);
    }
  } else {
    const char *badge = ctx.media_muted == 1 ? icon::VOLUME_OFF
                        : state == "playing" ? icon::PLAY
                        : state == "paused"  ? icon::PAUSE
                                             : icon::SPEAKER;
    draw_badge(it, f, badge, state == "playing");
    const std::string &title = str(ctx.selected_media_title);
    const std::string &artist = str(ctx.selected_media_artist);
    bool loaded = state == "playing" || state == "paused" || state == "buffering" || state == "on";
    if (loaded && !title.empty()) {
      text_fit(it, f.small, HERO_X, 36, TextAlign::BASELINE_LEFT, title.c_str(), SCREEN_W - HERO_X);
      if (!artist.empty()) {
        text_fit(it, f.tiny, HERO_X, 47, TextAlign::BASELINE_LEFT, artist.c_str(), SCREEN_W - HERO_X);
      }
    } else {
      char word[16];
      hero_word(it, f, label(state, word, sizeof(word), "SYNCING"));
    }
  }
  const char *feedback = nullptr;
  char buf[32];
  if (!is_tv) {
    feedback = fresh_feedback(ctx, ctx.last_media_power_feedback, ctx.last_media_power_interaction, buf, sizeof(buf));
  }
  if (draw_footer_overlay(it, f, ctx, feedback)) {
    return;
  }
  draw_setting_footer(it, f, ctx);
}

// A value too wide for the large font, in the title font: the value is
// shortened, never its unit.
void hero_word_with_unit(Display *it, const RemoteUiFonts &f, const char *value, const char *unit) {
  char unit_text[24];
  snprintf(unit_text, sizeof(unit_text), " %s", unit);
  int unit_w = unit[0] != '\0' ? text_width(f.title, unit_text) : 0;
  int room = SCREEN_W - HERO_X - unit_w;
  if (unit_w == 0 || room < 24) {
    hero_word(it, f, value);
    return;
  }
  char buf[64];
  const char *shown = fit_text(f.title, value, room, buf, sizeof(buf));
  text(it, f.title, HERO_X, HERO_WORD_BASELINE, TextAlign::BASELINE_LEFT, shown);
  text(it, f.title, HERO_X + text_width(f.title, shown), HERO_WORD_BASELINE, TextAlign::BASELINE_LEFT, unit_text);
}

// A number with its unit: big when it fits beside the unit, else in the title
// font. Rounded: Home Assistant keeps 15 significant digits of a float, so
// "23.6000003814697" is 23.6.
void hero_number_with_unit(Display *it, const RemoteUiFonts &f, double number, const char *unit) {
  char value[32];
  format_ha_number(number, value, sizeof(value));
  int unit_w = text_width(f.small, unit);
  int room = SCREEN_W - HERO_X - (unit_w > 0 ? unit_w + 2 : 0);
  if (has_glyphs(f.large, value) && text_width(f.large, value) <= room) {
    int right = hero_value(it, f, value);
    text(it, f.small, right + 2, HERO_BASELINE, TextAlign::BASELINE_LEFT, unit);
  } else {
    hero_word_with_unit(it, f, value, unit);
  }
}

// An enum sensor reports a key such as "not_charging": shown as words.
bool is_state_key(const std::string &state) {
  for (char c : state) {
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
      return false;
    }
  }
  return !state.empty();
}

// A person or device tracker: HOME, AWAY, or the zone it is in.
void render_presence(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool home = state == "home";
  draw_badge(it, f, home || ha_state_missing(state) ? icon::PERSON : icon::PERSON_AWAY, home);
  const char *word = presence_state_word(state);
  hero_word(it, f, ha_state_missing(state) ? missing_word(state) : word != nullptr ? word : state.c_str());
  draw_footer_overlay(it, f, ctx, nullptr);
}

void render_sensor(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  if (ctx.sensor_is_presence) {
    render_presence(it, f, ctx);
    return;
  }
  const std::string &state = str(ctx.selected_item_state);
  const std::string &unit = str(ctx.selected_sensor_unit);
  bool binary = state == "on" || state == "off";
  draw_badge(it, f, icon::SENSOR, state == "on");
  double number = 0;
  if (ha_state_missing(state)) {
    hero_word(it, f, missing_word(state));
  } else if (binary) {
    hero_value(it, f, state == "on" ? "ON" : "OFF");
  } else if (parse_ha_number(state, &number)) {
    hero_number_with_unit(it, f, number, unit.c_str());
  } else {
    char word[48];
    hero_word_with_unit(it, f, is_state_key(state) ? label(state, word, sizeof(word)) : state.c_str(), unit.c_str());
  }
  draw_footer_overlay(it, f, ctx, nullptr);
}

void render_automation(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const char *glyph = icon::SCRIPT;
  const char *kind = "SCRIPT";
  if (ctx.automation_kind == AUTOMATION_KIND_AUTOMATION) {
    glyph = icon::AUTOMATION;
    kind = "AUTOMATION";
  } else if (ctx.automation_kind == AUTOMATION_KIND_SCENE) {
    glyph = icon::SCENE;
    kind = "SCENE";
  } else if (ctx.automation_kind == AUTOMATION_KIND_BUTTON) {
    glyph = icon::TOUCH;
    kind = "BUTTON";
  }
  const std::string &state = str(ctx.selected_item_state);
  bool running = state == "on" && ctx.automation_kind == AUTOMATION_KIND_SCRIPT;
  draw_badge(it, f, glyph, running);
  draw_chip(it, f.tiny, HERO_X, 27, 10, kind, false);
  char word[16];
  if (ctx.automation_kind == AUTOMATION_KIND_AUTOMATION) {
    snprintf(word, sizeof(word), "%s", state == "off" ? "DISABLED" : "ENABLED");
  } else {
    snprintf(word, sizeof(word), "%s", running ? "RUNNING" : "READY");
  }
  text(it, f.title, HERO_X, 50, TextAlign::BASELINE_LEFT, ha_state_missing(state) ? missing_word(state) : word);
  char buf[32];
  if (draw_footer_overlay(it, f, ctx,
                          fresh_feedback(ctx, ctx.last_automation_feedback, ctx.last_automation_interaction, buf,
                                         sizeof(buf)))) {
    return;
  }
  footer_hints(it, f, nullptr, ctx.automation_kind == AUTOMATION_KIND_BUTTON ? "PRESS" : "RUN", true);
}

// Number and select entities: the value (with its unit) or the option, and
// the footer that Plus and Minus step.
void render_input(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  draw_badge(it, f, ctx.input_is_select ? icon::LIST : icon::TUNE, false);
  if (ha_state_missing(state)) {
    hero_word(it, f, missing_word(state));
  } else if (ctx.input_is_select) {
    char option[48];
    str_upper_to_buffer(state, option, sizeof(option));
    hero_word(it, f, option);
  } else if (std::isfinite(ctx.input_value)) {
    hero_number_with_unit(it, f, ctx.input_value, str(ctx.input_unit).c_str());
  } else {
    hero_word(it, f, state.c_str());
  }
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  draw_setting_footer(it, f, ctx);
}

// Vacuums and lawn mowers: the badge lights while one is at work. The footer
// offers start or pause and docking, or the fan speed once Settings picks it.
void render_vacuum(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool working = vacuum_state_working(state);
  draw_badge(it, f, ctx.vacuum_is_mower ? icon::MOWER : icon::VACUUM, working);
  char word[24];
  hero_word(it, f, ha_state_missing(state) ? missing_word(state) : label(state, word, sizeof(word)));
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  if (ctx.selected_setting_option == REMOTE_SETTING_VACUUM_FAN_SPEED) {
    draw_setting_footer(it, f, ctx);
  } else {
    footer_hints(it, f, "DOCK", working ? "PAUSE" : "START", false);
  }
}

// Timers count down on screen between Home Assistant's updates.
void render_timer(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool active = state == "active";
  bool paused = state == "paused";
  draw_badge(it, f, paused ? icon::TIMER_PAUSE : icon::TIMER, active);
  char word[16];
  if (ha_state_missing(state)) {
    hero_word(it, f, missing_word(state));
  } else if (ctx.timer_seconds < 0) {
    hero_word(it, f, label(state, word, sizeof(word)));
  } else {
    char countdown[24];
    format_countdown(ctx.timer_seconds, countdown, sizeof(countdown));
    int right = hero_value(it, f, countdown);
    label(active && ctx.timer_seconds == 0 ? std::string("done") : state, word, sizeof(word));
    if (text_width(f.tiny, word) <= SCREEN_W - right - 4) {
      hero_caption(it, f, 33, word, right + 4);
    }
  }
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  footer_hints(it, f, active || paused ? "CANCEL" : nullptr, active ? "PAUSE" : "START", false);
}

void render_alarm(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &state = str(ctx.selected_item_state);
  bool triggered = state == "triggered";
  bool armed = state.rfind("armed", 0) == 0;
  draw_badge(it, f, triggered ? icon::SHIELD_ALERT : armed ? icon::SHIELD_ARMED : icon::SHIELD, armed || triggered);
  char word[24];
  hero_word(it, f, label(state, word, sizeof(word), "SYNCING"));
  char buf[32];
  if (draw_footer_overlay(it, f, ctx,
                          fresh_feedback(ctx, ctx.last_alarm_feedback, ctx.last_alarm_interaction, buf, sizeof(buf)))) {
    return;
  }
  if (ctx.selected_setting_option == REMOTE_SETTING_ALARM_STATE) {
    draw_setting_footer(it, f, ctx);
  } else {
    footer_hints(it, f, "DISARM", "ARM", true);
  }
}

// Word-wraps a notification into up to max_lines lines; the last line is
// shortened with an ellipsis when the message does not fit.
void draw_wrapped(Display *it, font::Font *font, const char *message, int first_baseline, int line_height,
                  int max_lines) {
  const char *p = message;
  for (int line = 0; line < max_lines && *p != '\0'; line++) {
    while (*p == ' ' || *p == '\n') {
      p++;
    }
    // Take whole words while they fit.
    char row[96];
    size_t row_len = 0;
    const char *scan = p;
    const char *row_end = p;
    while (*scan != '\0' && *scan != '\n') {
      const char *word_end = scan;
      while (*word_end != '\0' && *word_end != ' ' && *word_end != '\n') {
        word_end++;
      }
      size_t take = word_end - p;
      if (take >= sizeof(row)) {
        break;
      }
      memcpy(row, p, take);
      row[take] = '\0';
      if (text_width(font, row) > SCREEN_W && row_end != p) {
        break;
      }
      row_end = word_end;
      row_len = take;
      scan = word_end;
      while (*scan == ' ') {
        scan++;
      }
    }
    if (row_end == p) {
      // A single word wider than the screen: let fit_text cut it.
      row_len = std::min(strcspn(p, " \n"), sizeof(row) - 1);
      row_end = p + row_len;
    }
    memcpy(row, p, row_len);
    row[row_len] = '\0';
    if (line == max_lines - 1 && *row_end != '\0') {
      // More text follows: show as much of the rest as fits, with an ellipsis.
      snprintf(row, sizeof(row), "%s", p);
      for (char *c = row; *c != '\0'; c++) {
        if (*c == '\n') {
          *c = ' ';
        }
      }
      text_fit(it, font, 0, first_baseline + line * line_height, TextAlign::BASELINE_LEFT, row, SCREEN_W);
      return;
    }
    text_fit(it, font, 0, first_baseline + line * line_height, TextAlign::BASELINE_LEFT, row, SCREEN_W);
    p = row_end;
  }
}

void render_notifications(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &message = str(ctx.selected_item_state);
  if (recent(ctx.now, ctx.last_notification_dismiss_interaction, TOAST_MS)) {
    draw_icon(it, f.hero, 64, 30, icon::ALL_CLEAR);
    footer_toast(it, f, "DISMISSED");
    return;
  }
  if (message.empty() || ha_state_missing(message)) {
    draw_icon(it, f.hero, 64, 28, icon::ALL_CLEAR);
    text(it, f.small, 64, 50, TextAlign::BASELINE_CENTER, "ALL CAUGHT UP");
    draw_footer_overlay(it, f, ctx, nullptr);
    return;
  }
  draw_wrapped(it, f.small, message.c_str(), 21, 11, 3);
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  footer_hints(it, f, nullptr, "DISMISS", false);
  if (ctx.item_count > 1) {
    int cy = FOOTER_Y + FOOTER_H / 2;
    draw_chevron(it, 1, cy, false);
    draw_chevron(it, 8, cy, true);
    text(it, f.tiny, 15, FOOTER_Y + 9, TextAlign::BASELINE_LEFT, "MORE");
  }
}

const char *weather_icon(const std::string &condition, bool night) {
  // Home Assistant reports clear-night after dark, so sunny is always the sun.
  // partlycloudy has no night form, so the clock decides.
  if (condition == "sunny") return icon::SUNNY;
  if (condition == "clear") return night ? icon::CLEAR_NIGHT : icon::SUNNY;  // not an HA condition, but seen
  if (condition == "clear-night") return icon::CLEAR_NIGHT;
  if (condition == "partlycloudy") return night ? icon::PARTLY_NIGHT : icon::PARTLY_DAY;
  if (condition == "cloudy") return icon::CLOUD;
  if (condition == "rainy" || condition == "pouring") return icon::RAIN;
  if (condition == "lightning" || condition == "lightning-rainy" || condition == "exceptional") return icon::STORM;
  if (condition == "snowy" || condition == "snowy-rainy") return icon::SNOW;
  if (condition == "hail") return icon::HAIL;
  if (condition == "fog") return icon::FOG;
  if (condition == "windy" || condition == "windy-variant") return icon::WIND;
  return icon::CLOUD;
}

void weather_condition_label(const std::string &raw, char *buf, size_t size) {
  if (raw == "partlycloudy") {
    snprintf(buf, size, "PARTLY CLOUDY");
    return;
  }
  if (raw == "clear-night") {
    snprintf(buf, size, "CLEAR");
    return;
  }
  remote_state_label_to_buffer(raw, buf, size, "");
  for (char *c = buf; *c != '\0'; c++) {
    if (*c == '-') {
      *c = ' ';
    }
  }
}

const char *compass_point(float bearing) {
  static const char *const POINTS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  if (!std::isfinite(bearing)) {
    return "";
  }
  float sector = std::fmod(bearing + 22.5f, 360.0f);
  if (sector < 0) {
    sector += 360.0f;
  }
  return POINTS[static_cast<int>(sector / 45.0f) % 8];
}

// A weather entity's wind speed unit as the footer prints it: "MPH", "KM/H".
const char *wind_unit_label(const char *unit, char *buf, size_t size) {
  if (strcmp(unit, "Beaufort") == 0) {
    return "BFT";
  }
  size_t i = 0;
  for (; unit[i] != '\0' && i + 1 < size; i++) {
    buf[i] = (unit[i] >= 'a' && unit[i] <= 'z') ? static_cast<char>(unit[i] - 'a' + 'A') : unit[i];
  }
  buf[i] = '\0';
  return buf;
}

// Weather details: +/- step through them, so they use the options footer, with
// a meter or a compass where a picture says more than the number. Each reading
// is in the unit the weather entity reports, with the decimals that unit needs
// ("29.92 inHg", "1013 hPa").
void draw_weather_footer(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  auto option = static_cast<RemoteSettingOption>(ctx.selected_setting_option);
  char value[32];
  const char *t = ctx.weather_temperature_unit != nullptr && ctx.weather_temperature_unit[0] != '\0'
                      ? ctx.weather_temperature_unit
                      : ctx.temperature_unit;
  char speed_unit[12];
  wind_unit_label(ctx.weather_speed_unit, speed_unit, sizeof(speed_unit));
  const int speed_decimals = weather_speed_decimals(ctx.weather_speed_unit);
  switch (option) {
    case REMOTE_SETTING_WEATHER_HUMIDITY:
      snprintf(value, sizeof(value), "%.0f%%", ctx.selected_weather_humidity);
      footer_range(it, f, "HUMIDITY", meter_percent(ctx.selected_weather_humidity),
                   or_dash(ctx.selected_weather_humidity, value));
      return;
    case REMOTE_SETTING_WEATHER_CLOUD_COVERAGE:
      snprintf(value, sizeof(value), "%.0f%%", ctx.selected_weather_cloud_coverage);
      footer_range(it, f, "CLOUDS", meter_percent(ctx.selected_weather_cloud_coverage),
                   or_dash(ctx.selected_weather_cloud_coverage, value));
      return;
    case REMOTE_SETTING_WEATHER_UV_INDEX:
      snprintf(value, sizeof(value), "%.1f", ctx.selected_weather_uv_index);
      footer_range(it, f, "UV INDEX", meter_percent(ctx.selected_weather_uv_index * 100 / 11),
                   or_dash(ctx.selected_weather_uv_index, value));
      return;
    case REMOTE_SETTING_WEATHER_WIND_SPEED:
    case REMOTE_SETTING_WEATHER_WIND_BEARING: {
      bool bearing_only = option == REMOTE_SETTING_WEATHER_WIND_BEARING;
      int x = footer_chip(it, f, bearing_only ? "WIND DIR" : "WIND");
      draw_chevron(it, x + 1, FOOTER_Y + FOOTER_H / 2, false);
      x += 7;
      bool has_bearing = std::isfinite(ctx.selected_weather_wind_bearing);
      if (has_bearing) {
        // Compass: the needle points where the wind is blowing to.
        int cx = x + 6, cy = FOOTER_Y + FOOTER_H / 2;
        it->circle(cx, cy, 5, ON);
        float rad = (ctx.selected_weather_wind_bearing + 180.0f) * static_cast<float>(M_PI) / 180.0f;
        it->line(cx, cy, cx + static_cast<int>(std::lround(std::sin(rad) * 4)),
                 cy - static_cast<int>(std::lround(std::cos(rad) * 4)), ON);
        x += 14;
      }
      if (bearing_only) {
        snprintf(value, sizeof(value), "%s %.0f°", compass_point(ctx.selected_weather_wind_bearing),
                 display_rounded(ctx.selected_weather_wind_bearing, 0));
      } else if (has_bearing) {
        snprintf(value, sizeof(value), "%.*f %s %s", speed_decimals,
                 display_rounded(ctx.selected_weather_wind_speed, speed_decimals), speed_unit,
                 compass_point(ctx.selected_weather_wind_bearing));
      } else {
        snprintf(value, sizeof(value), "%.*f %s", speed_decimals,
                 display_rounded(ctx.selected_weather_wind_speed, speed_decimals), speed_unit);
      }
      text_fit(it, f.tiny, (x + SCREEN_W - 6) / 2, FOOTER_Y + 9, TextAlign::BASELINE_CENTER,
               or_dash(bearing_only ? ctx.selected_weather_wind_bearing : ctx.selected_weather_wind_speed, value),
               SCREEN_W - x - 8);
      draw_chevron(it, SCREEN_W - 5, FOOTER_Y + FOOTER_H / 2, true);
      return;
    }
    case REMOTE_SETTING_WEATHER_WIND_GUST:
      snprintf(value, sizeof(value), "%.*f %s", speed_decimals,
               display_rounded(ctx.selected_weather_wind_gust_speed, speed_decimals), speed_unit);
      footer_options(it, f, "GUSTS", or_dash(ctx.selected_weather_wind_gust_speed, value));
      return;
    case REMOTE_SETTING_WEATHER_PRESSURE: {
      int decimals = weather_pressure_decimals(ctx.weather_pressure_unit);
      snprintf(value, sizeof(value), "%.*f %s", decimals, display_rounded(ctx.selected_weather_pressure, decimals),
               ctx.weather_pressure_unit);
      footer_options(it, f, "PRESSURE", or_dash(ctx.selected_weather_pressure, value));
      return;
    }
    case REMOTE_SETTING_WEATHER_PRECIPITATION: {
      // Rain in inches needs two decimals ("0.04 in"); millimetres one.
      int decimals = weather_precipitation_decimals(ctx.weather_precipitation_unit);
      snprintf(value, sizeof(value), "%.*f %s", decimals,
               display_rounded(ctx.selected_weather_precipitation, decimals), ctx.weather_precipitation_unit);
      footer_options(it, f, "PRECIP", or_dash(ctx.selected_weather_precipitation, value));
      return;
    }
    case REMOTE_SETTING_WEATHER_DEW_POINT:
      snprintf(value, sizeof(value), "%.0f°%s", display_rounded(ctx.selected_weather_dew_point, 0), t);
      footer_options(it, f, "DEW POINT", or_dash(ctx.selected_weather_dew_point, value));
      return;
    case REMOTE_SETTING_WEATHER_APPARENT_TEMP:
      snprintf(value, sizeof(value), "%.0f°%s", display_rounded(ctx.selected_weather_apparent_temperature, 0), t);
      footer_options(it, f, "FEELS LIKE", or_dash(ctx.selected_weather_apparent_temperature, value));
      return;
    case REMOTE_SETTING_WEATHER_HIGH_TEMP:
      snprintf(value, sizeof(value), "%.0f°%s", display_rounded(ctx.selected_weather_high_temp, 0), t);
      footer_options(it, f, "HIGH", or_dash(ctx.selected_weather_high_temp, value));
      return;
    case REMOTE_SETTING_WEATHER_LOW_TEMP:
      snprintf(value, sizeof(value), "%.0f°%s", display_rounded(ctx.selected_weather_low_temp, 0), t);
      footer_options(it, f, "LOW", or_dash(ctx.selected_weather_low_temp, value));
      return;
    case REMOTE_SETTING_WEATHER_CONDITIONS:
    default: {
      char condition[24];
      weather_condition_label(str(ctx.selected_weather_condition), condition, sizeof(condition));
      footer_options(it, f, "NOW", condition[0] != '\0' ? condition : "SYNCING");
      return;
    }
  }
}

void render_weather(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const std::string &condition = str(ctx.selected_weather_condition);
  bool known_condition = !ha_state_missing(condition);
  bool has_temperature = !std::isnan(ctx.selected_weather_temperature);
  if (condition == "unavailable" || (!known_condition && !has_temperature)) {
    draw_icon(it, f.hero, HERO_CX, HERO_CY, icon::CLOUD);
    hero_word(it, f, missing_word(condition));
  } else if (!has_temperature) {
    // An entity that reports conditions but no temperature.
    draw_icon(it, f.hero, HERO_CX, HERO_CY, weather_icon(condition, ctx.weather_is_night));
    char word[24];
    weather_condition_label(condition, word, sizeof(word));
    hero_word(it, f, word);
  } else {
    // A temperature with an unknown condition still shows, under a cloud.
    draw_icon(it, f.hero, HERO_CX, HERO_CY,
              known_condition ? weather_icon(condition, ctx.weather_is_night) : icon::CLOUD);
    char temp[12];
    format_temp(temp, sizeof(temp), ctx.selected_weather_temperature);
    hero_value(it, f, temp);
    // Today's high and low, each marked with a small up/down arrowhead.
    char value[12];
    if (!std::isnan(ctx.selected_weather_high_temp)) {
      format_temp(value, sizeof(value), ctx.selected_weather_high_temp);
      text(it, f.small, SCREEN_W, 36, TextAlign::BASELINE_RIGHT, value);
      int x = SCREEN_W - text_width(f.small, value) - 7;
      it->filled_triangle(x, 35, x + 4, 35, x + 2, 31, ON);
    }
    if (!std::isnan(ctx.selected_weather_low_temp)) {
      format_temp(value, sizeof(value), ctx.selected_weather_low_temp);
      text(it, f.small, SCREEN_W, HERO_BASELINE, TextAlign::BASELINE_RIGHT, value);
      int x = SCREEN_W - text_width(f.small, value) - 7;
      it->filled_triangle(x, 43, x + 4, 43, x + 2, 47, ON);
    }
  }
  if (draw_footer_overlay(it, f, ctx, nullptr)) {
    return;
  }
  draw_weather_footer(it, f, ctx);
}

void render_info(Display *it, const RemoteUiFonts &f, const RemoteRenderContext &ctx) {
  const char *primary = ctx.info_primary_text.c_str();
  const char *secondary = ctx.info_secondary_text.c_str();
  switch (ctx.info_index) {
    case 0: {  // Time & date: the clock is the hero, the date the title.
      draw_badge(it, f, icon::CLOCK, false);
      if (!ctx.clock_valid) {
        hero_value(it, f, "--:--");
        break;
      }
      char clock[12];
      int hour = ctx.clock_hour % 12;
      snprintf(clock, sizeof(clock), "%d:%02d", hour == 0 ? 12 : hour, ctx.clock_minute);
      int right = hero_value(it, f, clock);
      text(it, f.small, right + 2, HERO_BASELINE, TextAlign::BASELINE_LEFT, ctx.clock_hour >= 12 ? "PM" : "AM");
      break;
    }
    case 1:  // Wireless
      draw_badge(it, f, ctx.wifi_rssi == 0 ? icon::WIFI_OFF : icon::WIFI, ctx.wifi_rssi != 0);
      text_fit(it, f.title, HERO_X, 36, TextAlign::BASELINE_LEFT, primary, SCREEN_W - HERO_X);
      draw_signal_bars(it, HERO_X, 49, ctx.wifi_rssi);
      text(it, f.tiny, HERO_X + 23, 49, TextAlign::BASELINE_LEFT, secondary);
      break;
    case 2:  // Network
      draw_badge(it, f, icon::LAN, false);
      text(it, f.tiny, HERO_X, 33, TextAlign::BASELINE_LEFT, "IP ADDRESS");
      text_fit(it, f.title, HERO_X, 47, TextAlign::BASELINE_LEFT, primary, SCREEN_W - HERO_X);
      break;
    case 3:  // Device name
      draw_badge(it, f, icon::DEVICE, false);
      text_fit(it, f.title, HERO_X, 38, TextAlign::BASELINE_LEFT, primary, SCREEN_W - HERO_X);
      text_fit(it, f.tiny, HERO_X, 49, TextAlign::BASELINE_LEFT, secondary, SCREEN_W - HERO_X);
      break;
    case 4: {  // Battery: a big battery filled to the charge level.
      if (!ctx.battery_monitoring_available) {
        draw_badge(it, f, icon::BATTERY, false);
        hero_word(it, f, "UNAVAILABLE");
        break;
      }
      const int x = 1, y = 29, w = 23, h = 17;
      it->rectangle(x, y, w, h, ON);
      it->filled_rectangle(x + w, y + 5, 2, h - 10, ON);
      int fill = (w - 4) * std::max(0, std::min(100, ctx.battery_percentage)) / 100;
      if (fill > 0) {
        it->filled_rectangle(x + 2, y + 2, fill, h - 4, ON);
      }
      char pct[12];
      snprintf(pct, sizeof(pct), "%d%%", ctx.battery_percentage);
      hero_value(it, f, pct);
      if (!std::isnan(ctx.battery_voltage)) {
        char volts[12];
        snprintf(volts, sizeof(volts), "%.2f V", ctx.battery_voltage);
        hero_caption(it, f, HERO_BASELINE, volts, 96);
      }
      break;
    }
    default: {  // Version
      draw_badge(it, f, icon::INFO, false);
      int right = hero_value(it, f, primary);
      text(it, f.tiny, SCREEN_W, 33, TextAlign::BASELINE_RIGHT, "ESPHOME");
      hero_caption(it, f, HERO_BASELINE, secondary, right + 4);
      break;
    }
  }
  draw_footer_overlay(it, f, ctx, nullptr);
}

}  // namespace

void render_remote_ui(display::Display *it, const RemoteUiFonts &fonts, const RemoteRenderContext &ctx) {
  it->clear();
  draw_header(it, fonts, ctx);
  if (ctx.mode == REMOTE_MODE_INFO && ctx.info_index == 0 && ctx.clock_valid && ctx.clock_weekday >= 1 &&
      ctx.clock_weekday <= 7 && ctx.clock_month >= 1 && ctx.clock_month <= 12) {
    static const char *const DAYS[] = {"", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char *const MONTHS[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char date[32];
    snprintf(date, sizeof(date), "%s, %s %d", DAYS[ctx.clock_weekday], MONTHS[ctx.clock_month], ctx.clock_day);
    draw_name(it, fonts, date);
  } else if (ctx.mode != REMOTE_MODE_NOTIFICATIONS) {
    draw_name(it, fonts, str(ctx.selected_item_name).c_str());
  }
  switch (ctx.mode) {
    case REMOTE_MODE_LIGHTS:
      render_light(it, fonts, ctx);
      break;
    case REMOTE_MODE_SWITCHES:
      render_switch(it, fonts, ctx);
      break;
    case REMOTE_MODE_CLIMATE:
      render_climate(it, fonts, ctx);
      break;
    case REMOTE_MODE_WATER_HEATERS:
      render_water_heater(it, fonts, ctx);
      break;
    case REMOTE_MODE_HUMIDIFIERS:
      render_humidifier(it, fonts, ctx);
      break;
    case REMOTE_MODE_FANS:
      render_fan(it, fonts, ctx);
      break;
    case REMOTE_MODE_COVERS:
      render_cover(it, fonts, ctx);
      break;
    case REMOTE_MODE_LOCKS:
      render_lock(it, fonts, ctx);
      break;
    case REMOTE_MODE_MEDIA:
      render_media(it, fonts, ctx);
      break;
    case REMOTE_MODE_SENSORS:
      render_sensor(it, fonts, ctx);
      break;
    case REMOTE_MODE_AUTOMATION:
      render_automation(it, fonts, ctx);
      break;
    case REMOTE_MODE_ALARMS:
      render_alarm(it, fonts, ctx);
      break;
    case REMOTE_MODE_NOTIFICATIONS:
      render_notifications(it, fonts, ctx);
      break;
    case REMOTE_MODE_WEATHER:
      render_weather(it, fonts, ctx);
      break;
    case REMOTE_MODE_INPUTS:
      render_input(it, fonts, ctx);
      break;
    case REMOTE_MODE_VACUUMS:
      render_vacuum(it, fonts, ctx);
      break;
    case REMOTE_MODE_TIMERS:
      render_timer(it, fonts, ctx);
      break;
    case REMOTE_MODE_INFO:
    default:
      render_info(it, fonts, ctx);
      break;
  }
}

void remote_ui_header_clock_box(int *x, int *width) {
  *x = last_clock_x;
  *width = last_clock_w;
}

void render_snapshot_status(display::Display *it, const RemoteUiFonts &fonts, const char *status) {
  footer_toast(it, fonts, status);
}

void render_system_screen(display::Display *it, const RemoteUiFonts &fonts, RemoteSystemScreen screen,
                          const RemoteSystemScreenInfo &info) {
  it->clear();
  const RemoteUiFonts &f = fonts;
  const char *glyph = icon::WIFI;
  const char *headline = "";
  const char *detail = "";
  bool lit = false;
  switch (screen) {
    case REMOTE_SCREEN_CONNECTING_WIFI:
      glyph = icon::WIFI;
      headline = "WI-FI";
      detail = "CONNECTING\u2026";
      break;
    case REMOTE_SCREEN_CONNECTING_API:
      glyph = icon::HOME;
      headline = "HOME ASSISTANT";
      detail = "CONNECTING\u2026";
      break;
    case REMOTE_SCREEN_WIFI_LOST:
      glyph = icon::WIFI_OFF;
      headline = "WI-FI LOST";
      detail = "RECONNECTING\u2026";
      break;
    case REMOTE_SCREEN_API_LOST:
      glyph = icon::CLOUD_OFF;
      headline = "HOME ASSISTANT";
      detail = "RECONNECTING\u2026";
      break;
    case REMOTE_SCREEN_LOW_BATTERY:
      glyph = icon::BATTERY_ALERT;
      headline = "LOW BATTERY";
      detail = "PLEASE CHARGE";
      lit = true;
      break;
    case REMOTE_SCREEN_POWERING_OFF:
      glyph = icon::POWER;
      headline = "GOODBYE";
      detail = "POWERING OFF";
      break;
    case REMOTE_SCREEN_HOLD_TO_REBOOT:
      glyph = icon::RESTART;
      headline = "HOLD TO REBOOT";
      detail = "RELEASE TO SLEEP";
      break;
    case REMOTE_SCREEN_REBOOTING:
      glyph = icon::RESTART;
      headline = "REBOOTING";
      detail = "BE RIGHT BACK";
      lit = true;
      break;
  }
  draw_badge(it, f, glyph, lit, 64, 15, 13);
  text_fit(it, f.title, 64, 42, TextAlign::BASELINE_CENTER, headline, SCREEN_W);

  if (screen == REMOTE_SCREEN_HOLD_TO_REBOOT && info.progress >= 0) {
    draw_meter(it, f.tiny, 10, 50, SCREEN_W - 20, 11, info.progress, detail);
    return;
  }
  if (screen == REMOTE_SCREEN_LOW_BATTERY && !std::isnan(info.battery_voltage)) {
    char line[32];
    snprintf(line, sizeof(line), "%s  %.2fV", detail, info.battery_voltage);
    text(it, f.tiny, 64, 56, TextAlign::BASELINE_CENTER, line);
    return;
  }
  text_fit(it, f.tiny, 64, 56, TextAlign::BASELINE_CENTER, detail, SCREEN_W);
  if ((screen == REMOTE_SCREEN_CONNECTING_WIFI || screen == REMOTE_SCREEN_CONNECTING_API) && info.version != nullptr &&
      info.version[0] != '\0') {
    char version[16];
    snprintf(version, sizeof(version), "V%s", info.version);
    text(it, f.tiny, SCREEN_W, 63, TextAlign::BASELINE_RIGHT, version);
  }
}

}  // namespace esphome
