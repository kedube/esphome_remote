#include "oled_snapshot.h"

#include <cstring>

#include "esp_attr.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {

namespace {

// DisplayBuffer::buffer_ is protected with no public accessor. Explicit
// template instantiation may name protected members ([temp.explicit] exempts
// it from access checking), which yields a legal member pointer; the
// framebuffer download in framebuffer_web_debug.cpp reads it the same way.
template <typename Tag> struct StolenMember {
  static typename Tag::type value;
};
template <typename Tag> typename Tag::type StolenMember<Tag>::value;

template <typename Tag, typename Tag::type Ptr> struct StealMember {
  StealMember() { StolenMember<Tag>::value = Ptr; }
  static StealMember instance;
};
template <typename Tag, typename Tag::type Ptr> StealMember<Tag, Ptr> StealMember<Tag, Ptr>::instance;

struct DisplayBufferMember {
  using type = uint8_t *display::DisplayBuffer::*;
};
template struct StealMember<DisplayBufferMember, &display::DisplayBuffer::buffer_>;

uint8_t *framebuffer(display::DisplayBuffer *display) { return display->*StolenMember<DisplayBufferMember>::value; }

// The SH1106's 128x64 frame, one bit per pixel.
constexpr size_t FRAME_BYTES = 128 * 64 / 8;
constexpr uint32_t SNAPSHOT_MAGIC = 0x52454D31;  // "REM1"

struct Snapshot {
  uint32_t magic;
  uint32_t checksum;
  int16_t clock_x;
  int16_t clock_width;
  uint32_t entity_hash;  // the item on screen, for oled_snapshot_shows
  int16_t setting;       // the setting drawn
  int16_t preferred;     // the setting last picked with Settings
  uint8_t frame[FRAME_BYTES];
};

// RTC slow memory: kept through deep sleep, set to zero by every other kind of
// start. Keeping it powered while asleep costs a few microamps.
RTC_DATA_ATTR Snapshot rtc_snapshot;

uint32_t frame_checksum(const Snapshot &snapshot) {
  uint32_t sum = 2166136261u;  // FNV-1a
  auto mix = [&sum](uint8_t byte) {
    sum ^= byte;
    sum *= 16777619u;
  };
  for (uint8_t byte : snapshot.frame) {
    mix(byte);
  }
  mix(static_cast<uint8_t>(snapshot.clock_x));
  mix(static_cast<uint8_t>(snapshot.clock_width));
  for (int shift = 0; shift < 32; shift += 8) {
    mix(static_cast<uint8_t>(snapshot.entity_hash >> shift));
  }
  mix(static_cast<uint8_t>(snapshot.setting));
  mix(static_cast<uint8_t>(snapshot.preferred));
  return sum;
}

uint32_t text_hash(const std::string &text) {
  uint32_t sum = 2166136261u;  // FNV-1a
  for (char c : text) {
    sum ^= static_cast<uint8_t>(c);
    sum *= 16777619u;
  }
  return sum;
}

bool display_matches(display::DisplayBuffer *display) {
  return display != nullptr && framebuffer(display) != nullptr && display->get_width() * display->get_height() / 8 ==
                                                                      static_cast<int>(FRAME_BYTES);
}

}  // namespace

void oled_snapshot_capture(display::DisplayBuffer *display, int clock_x, int clock_width, const std::string &entity,
                           int setting, int preferred) {
  if (!display_matches(display)) {
    return;
  }
  rtc_snapshot.magic = 0;
  memcpy(rtc_snapshot.frame, framebuffer(display), FRAME_BYTES);
  rtc_snapshot.clock_x = static_cast<int16_t>(clock_x);
  rtc_snapshot.clock_width = static_cast<int16_t>(clock_width);
  rtc_snapshot.entity_hash = text_hash(entity);
  rtc_snapshot.setting = static_cast<int16_t>(setting);
  rtc_snapshot.preferred = static_cast<int16_t>(preferred);
  rtc_snapshot.checksum = frame_checksum(rtc_snapshot);
  rtc_snapshot.magic = SNAPSHOT_MAGIC;
}

bool oled_snapshot_valid() {
  return rtc_snapshot.magic == SNAPSHOT_MAGIC && rtc_snapshot.checksum == frame_checksum(rtc_snapshot);
}

bool oled_snapshot_restore(display::DisplayBuffer *display) {
  if (!oled_snapshot_valid() || !display_matches(display)) {
    return false;
  }
  memcpy(framebuffer(display), rtc_snapshot.frame, FRAME_BYTES);
  if (rtc_snapshot.clock_width > 0) {
    // Drawn through the display, which applies its rotation.
    display->filled_rectangle(rtc_snapshot.clock_x, 0, rtc_snapshot.clock_width, 9, display::COLOR_OFF);
  }
  return true;
}

bool oled_snapshot_shows(const std::string &entity, int preferred) {
  return oled_snapshot_valid() && !entity.empty() && rtc_snapshot.entity_hash == text_hash(entity) &&
         rtc_snapshot.preferred == preferred;
}

int oled_snapshot_setting() { return oled_snapshot_valid() ? rtc_snapshot.setting : -1; }

}  // namespace esphome
