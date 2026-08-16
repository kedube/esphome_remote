// ============================================================================
// OLED deep-sleep hold release.
//
// This is a FIX, not debug code — it must survive removal of any temporary
// instrumentation.
//
// ESPHome's deep_sleep component latches the wakeup pin on the way into sleep
// with gpio_hold_en() + gpio_deep_sleep_hold_en() (see deep_sleep_esp32.cpp)
// and never releases the latch on wake. Because the sleep path drives
// PIN_OLED_EN low immediately before sleeping, the OLED enable line stays
// frozen low across the wake: the GPIOSwitch writes its register, the pad
// ignores it, and the panel never powers up.
//
// PIN_OLED_EN (GPIO15) is RTC-capable (RTCIO channel 13), so the hold lives in
// the RTC domain. gpio_hold_dis() alone does not clear it — the pad also needs
// rtc_gpio_hold_dis() and must be handed back to the digital IO mux with
// rtc_gpio_deinit().
// ============================================================================
#pragma once

#include <driver/gpio.h>
#include <driver/rtc_io.h>

inline void oled_release_deep_sleep_hold(int oled_en_pin) {
  const gpio_num_t pin = static_cast<gpio_num_t>(oled_en_pin);

  // Clear the global deep-sleep latch first.
  gpio_deep_sleep_hold_dis();

  // Then the RTC-domain hold for this specific pad, and hand it back to the
  // digital IO mux so gpio_config()/output writes take effect again.
  if (rtc_gpio_is_valid_gpio(pin)) {
    rtc_gpio_hold_dis(pin);
    rtc_gpio_deinit(pin);
  }

  // Finally the digital-domain hold.
  gpio_hold_dis(pin);
}
