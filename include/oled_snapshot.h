#pragma once

#include <cstdint>
#include <string>

namespace esphome {
namespace display {
class DisplayBuffer;
}

// The last UI frame, kept in RTC memory through deep sleep so the remote can
// show the item it wakes into straight away, instead of a connecting screen,
// while Wi-Fi and Home Assistant connect. RTC memory survives deep sleep only:
// after a power cut, a reboot or an update there is no snapshot.

// After Home Assistant connects, the snapshot stays up until the selected
// item's state arrives, but no longer than this.
inline constexpr uint32_t SNAPSHOT_SYNC_HOLD_MS = 2000;

// Copies the frame just drawn. clock_x and clock_width say where the header's
// clock is (width 0 for none): it would be out of date on waking, so it is
// blanked when the frame comes back. entity is the selected item, setting the
// setting drawn for it (selected_setting_option), and preferred the one last
// picked with Settings (preferred_setting_option).
void oled_snapshot_capture(display::DisplayBuffer *display, int clock_x, int clock_width, const std::string &entity,
                           int setting, int preferred);

// Whether a frame from before sleep is waiting.
bool oled_snapshot_valid();

// Puts that frame back in the display's buffer, without the clock. The caller
// draws over it and sends it to the panel. Returns false when there is none.
bool oled_snapshot_restore(display::DisplayBuffer *display);

// Whether the frame from before sleep shows this item, with preferred still
// the setting last picked (Settings hasn't been pressed since): a press made
// while it is on screen acts on what it shows.
bool oled_snapshot_shows(const std::string &entity, int preferred);

// The setting the frame from before sleep shows; -1 when there is none.
int oled_snapshot_setting();

// Whether the frame from before sleep shows this entity.
bool oled_snapshot_entity_is(const char *entity);

// Forgets the frame from before sleep.
void oled_snapshot_discard();

}  // namespace esphome
