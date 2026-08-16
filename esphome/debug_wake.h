// ============================================================================
// TEMPORARY DISPLAY-INIT DEBUGGING — remove together with
// packages/debug_wake.yaml.
//
// PASSIVE ONLY. This header observes state; it must never drive a peripheral.
//
// An earlier version polled the OLED with the raw IDF I2C driver
// (i2c_master_cmd_begin on I2C_NUM_0) to time the panel's power-up. That was a
// mistake: ESPHome's i2c component owns that peripheral and initializes it at
// BUS priority, so bypassing its locking could wedge the bus and kill the
// display outright — a worse failure than the one being debugged. Removed.
//
// What remains is safe: reading a GPIO level, reading the display component's
// own is_failed() flag, and counting boots in RTC memory (which survives deep
// sleep, unlike ESPHome globals).
// ============================================================================
#pragma once

#include "esphome.h"
#include <esp_sleep.h>
#include <driver/gpio.h>

static RTC_DATA_ATTR uint32_t debug_boot_count_rtc = 0;
static RTC_DATA_ATTR int8_t debug_last_boot_failed_rtc = -1;  // 1 = display FAILED on previous boot

static int debug_oled_pad_at_boot = -1;
// Whether the pad was genuinely HELD by the deep-sleep latch, as opposed to
// merely reading low because nothing has driven it yet. gpio_hold_dis() only
// means something after a deep-sleep wake, so this is only meaningful when the
// wakeup cause is EXT0/TIMER — on a power-on reset there is no latch at all.
static int debug_pad_was_held = -1;

inline uint32_t debug_get_boot_count() { return debug_boot_count_rtc; }
inline void debug_record_boot() { debug_boot_count_rtc++; }

// Reading a pad level is passive and safe at any boot priority.
//
// IMPORTANT: a LOW reading here is only evidence of the deep-sleep latch when
// the chip actually woke from deep sleep. On a power-on reset the pad reads LOW
// simply because oled_power has not been switched on yet (that happens at
// priority 700, below this hook). Record the wakeup cause alongside the level
// so the two cases can be told apart.
inline void debug_capture_boot_pad(int oled_pin) {
  debug_oled_pad_at_boot = gpio_get_level(static_cast<gpio_num_t>(oled_pin));

  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  const bool from_deep_sleep =
      (cause == ESP_SLEEP_WAKEUP_EXT0 || cause == ESP_SLEEP_WAKEUP_TIMER);
  debug_pad_was_held = from_deep_sleep ? (debug_oled_pad_at_boot == 0 ? 1 : 0) : -1;
}

inline void debug_record_display_failed(bool failed) {
  debug_last_boot_failed_rtc = failed ? 1 : 0;
}

inline std::string debug_report_string() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  const char *cause_str;
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0:      cause_str = "EXT0(button)"; break;
    case ESP_SLEEP_WAKEUP_TIMER:     cause_str = "TIMER"; break;
    case ESP_SLEEP_WAKEUP_UNDEFINED: cause_str = "UNDEFINED(reset/power-on)"; break;
    default:                         cause_str = "OTHER"; break;
  }

  const char *prev_failed = debug_last_boot_failed_rtc < 0 ? "unknown"
                          : (debug_last_boot_failed_rtc ? "FAILED" : "ok");

  const char *pad_str;
  if (debug_oled_pad_at_boot < 0) {
    pad_str = "unread";
  } else if (debug_pad_was_held < 0) {
    // Not a deep-sleep wake, so the level says nothing about the latch.
    pad_str = debug_oled_pad_at_boot ? "HIGH(n/a - not a sleep wake)"
                                     : "LOW(n/a - not a sleep wake)";
  } else if (debug_pad_was_held == 1) {
    pad_str = "LOW(LATCHED by deep sleep)";
  } else {
    pad_str = "HIGH(not latched)";
  }

  char buf[256];
  snprintf(buf, sizeof(buf),
           "boot=%u cause=%s oled_pad_at_boot=%s prev_boot_display=%s",
           (unsigned) debug_boot_count_rtc,
           cause_str,
           pad_str,
           prev_failed);
  return std::string(buf);
}
