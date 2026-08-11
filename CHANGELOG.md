# Changelog

Notable changes for each tagged release. Versions correspond to git tags and to the
`VERSION` substitution in `esphome/settings.yaml`, which the remote shows on its Info
screen. Add entries under **Unreleased** as part of each change; the release workflow
rotates that section into a version heading and publishes it as the release's Highlights.

## 3.3 — 2026-08-11
- Documented the optional media-player `sources` field on favorite entries, the ESP32
  `minimum_chip_revision` setting and the OTA failure it causes when mismatched, the
  water heater's target temperature range, and the requirement to select the alarm-state
  view before `Plus` / `Minus` change the arm mode.
- Corrected the `settings.yaml` comment that referred to a `REMOTE_FRAMEBUFFER_WEB_DEBUG`
  substitution; the substitution is named `FRAMEBUFFER_WEB_DEBUG`.
- Added a troubleshooting entry for an OTA update that reaches 100% and then fails.

## 3.2 — 2026-08-11
- Fixed a stack buffer overflow that could crash or reboot-loop the remote when opening
  the Weather settings view: the option list was built in an 8-slot array that a weather
  entity reporting 8 or more attributes overflowed.
- Fixed inverted cover prompts — the screen offered "HOLD TO OPEN" on the button that
  closes the cover, and vice versa.
- Fixed the alarm panic hold: holding Settings in alarm mode showed "HOLD TO TRIGGER"
  but cycled the settings view instead of triggering the alarm. Releasing the button
  early still cycles settings as before.
- Fixed the Square button in automation mode showing "HOLD TO RUN" and then doing
  nothing; only Circle runs an automation, and only Circle now offers the prompt.
- Removed the unreachable `dispatch_mode_action` command tree, whose lock/unlock and
  open/close mappings contradicted the live button wiring.
- Notification feed payloads are now size-capped like every other Home Assistant
  attribute, and the notification line splitter can no longer overflow its row buffer on
  malformed UTF-8.
- Corrected `DEEP_SLEEP_DURATION` guidance: setting it to `0` makes the device sleep
  immediately on boot rather than disabling forced sleep.
- Media players that are not a TV or receiver can list their sources with an optional
  third field on a favorite entry, which previously had no effect.
- An unavailable media player no longer displays as "ON", and automation feedback no
  longer carries over to the next selected item.
- Fixed a forced power-off after roughly 49.7 days of uptime at the `millis()` rollover.
- Added GitHub Actions CI (config validation and a firmware build for every PCB
  revision) and an automated release workflow that publishes prebuilt firmware.
