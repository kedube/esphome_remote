// ============================================================================
// TEMPORARY DISPLAY-INIT DEBUGGING — remove together with
// packages/debug_wake.yaml.
//
// Round 2. Round 1 proved the GPIO hold latch was real (oled_pad_at_boot=LOW)
// and releasing it was necessary but NOT sufficient — the display still fails
// to initialize after deep sleep.
//
// WHAT THIS ROUND TESTS: ssd1306_i2c::setup() probes the panel exactly ONCE
// (`write(nullptr, 0)`); on no-ACK it calls mark_failed(), which is terminal
// in ESPHome — update() never runs again for that boot, no retry ever happens.
// No reset_pin is configured, so init_reset_() is a no-op and the panel gets
// zero settle time inside setup(). Its only settle is the 10ms in on_boot.
//
// So the hypothesis is a RACE: the SH1106 hasn't finished powering up when
// that single probe fires. This records whether the panel ACKs on I2C at boot,
// how long it takes to start ACKing, and whether the display component ended
// up in the FAILED state — turning "the display didn't come up" into a number.
// ============================================================================
#pragma once

#include "esphome.h"
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

static RTC_DATA_ATTR uint32_t debug_boot_count_rtc = 0;
static RTC_DATA_ATTR int8_t debug_last_boot_failed_rtc = -1;  // 1 = display FAILED last boot

// Filled during boot.
static int debug_oled_pad_at_boot = -1;
static int debug_ack_ms = -1;        // ms after power-on until panel first ACKed (-1 = never)
static int debug_ack_attempts = 0;   // how many probes were needed

inline uint32_t debug_get_boot_count() { return debug_boot_count_rtc; }
inline void debug_record_boot() { debug_boot_count_rtc++; }

inline void debug_capture_boot_pad(int oled_pin) {
  debug_oled_pad_at_boot = gpio_get_level(static_cast<gpio_num_t>(oled_pin));
}

// Polls the OLED's I2C address directly, using the raw IDF driver rather than
// ESPHome's i2c component, so it works regardless of what state that component
// is in. Measures how long the panel actually takes to become responsive after
// power is applied — the number that decides how much settle delay is needed.
//
// Call this right after powering the rail, BEFORE the display component sets up.
inline void debug_measure_panel_ack(int address, int timeout_ms) {
  const uint32_t start = millis();
  debug_ack_ms = -1;
  debug_ack_attempts = 0;

  while ((int) (millis() - start) < timeout_ms) {
    debug_ack_attempts++;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);

    if (err == ESP_OK) {
      debug_ack_ms = (int) (millis() - start);
      return;
    }
    delay(5);
  }
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

  char buf[320];
  snprintf(buf, sizeof(buf),
           "boot=%u cause=%s oled_pad=%s panel_ack=%s attempts=%d",
           (unsigned) debug_boot_count_rtc,
           cause_str,
           debug_oled_pad_at_boot < 0 ? "unread"
             : (debug_oled_pad_at_boot ? "HIGH(powered)" : "LOW(latched off)"),
           debug_ack_ms < 0 ? "NEVER(panel dead)" : ([] {
             static char t[24];
             snprintf(t, sizeof(t), "%dms", debug_ack_ms);
             return (const char *) t;
           })(),
           debug_ack_attempts);
  return std::string(buf);
}
