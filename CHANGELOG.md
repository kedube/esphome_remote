# Changelog

Notable changes for each tagged release. Versions correspond to git tags and to the
`VERSION` substitution in `esphome/settings.yaml`, which the remote shows on its Info
screen. Add entries under **Unreleased** as part of each change; the release workflow
rotates that section into a version heading and publishes it as the release's Highlights.

## Unreleased
- Fixed CI's firmware builds failing whenever the build cache already matched the
  code, such as after a change to only the docs or workflows, and in the weekly build.

## 4.0 — 2026-10-02
- Redesigned the screen. Every page now uses the same layout: a header with the list
  name, your position in the list, the clock and battery; the entity name; a large
  value beside an icon badge that lights up when the device is on; and a footer that
  shows what the buttons do.
- Values are drawn as pictures where that is clearer: meters for brightness, speed,
  volume and position, a toggle switch for switches, a window with its shade for
  covers, speed steps for fans, a compass for wind, signal bars for Wi-Fi and a battery
  gauge.
- Protected actions (locks, covers, automations, alarm) fill a hold bar in the footer
  while held, so you can see when the action will fire. Holding the power button
  shows the same kind of bar before a reboot.
- Action results and messages such as `ALREADY ON` appear in the footer for a few
  seconds instead of replacing the detail line.
- The alarm footer shows all four arm modes with the selected one highlighted;
  `Plus` / `Minus` move the highlight.
- Notifications wrap at word boundaries instead of cutting words at fixed widths.
- Names and states that have to be shortened no longer end in half of an accented
  letter, symbol or emoji, which made the remote read past the end of the text.
- Fixed `Plus` / `Minus` doing nothing after the remote woke until `Settings` was pressed.
  The remote chose the item's setting before Home Assistant had sent any values, found
  none, and never looked again. It now chooses again as values arrive, returns to the
  setting you last picked with `Settings`, and keeps that choice across sleep.
- Fixed `Circle` not turning on a thermostat that was off: Home Assistant sends a
  thermostat's mode list in a form the remote couldn't read, so it only showed `SYNCING`.
- A favorite with no `entity_id`, or one in a domain the remote doesn't support, now
  stops the build with a clear message. It used to show an empty page that `Previous` /
  `Next` couldn't leave, and a missing `entity_id` crashed the remote at boot.
- Fixed thermostats that have a single setpoint being sent a low/high range (which Home
  Assistant rejects) when adjusted in heat/cool mode, and a temperature of `nan` being
  sent while a thermostat reports no target.
- Volume no longer starts from the previously selected player's level: a player that
  reports no volume shows `--`, and `Plus` / `Minus` ask it to step up or down. A
  player's title and artist also clear when it stops or turns off.
- Fans step by their own speed increments, so `Minus` works on fans with only a few
  speeds (a 10% step used to round back to the same speed).
- Fixed cover tilt stepping from another cover's tilt, a TV being sent an empty source
  after its source list emptied, and a partly open cover reporting `OPENED`.
- Water heater targets stay within the heater's own `min_temp` / `max_temp` instead of a
  fixed 90–175 °F (30–80 °C), which Home Assistant passes to the heater unchecked.
- While a light or fan is off, `Plus` turns it on; effects, presets, oscillation and
  direction appear once it is on. An off light with effects used to change its effect.
- Action buttons do nothing until Home Assistant is connected. Home Assistant drops
  actions sent earlier, so the remote showed changes that never happened.
- Fixed releasing the power button just after the reboot bar filled powering the remote
  off instead of rebooting it.
- The connecting and Wi-Fi-lost screens no longer draw over `GOODBYE`, `REBOOTING` or the
  reboot bar, and the main screen no longer flashes up before Home Assistant connects.
- The low-battery warning now appears even when Wi-Fi or Home Assistant can't connect.
- The selected setting and arm mode are saved on every shutdown, including the forced
  sleep after `DEEP_SLEEP_DURATION` and reboots, not only when powering off.
- Rev 3.1: the remote now actually waits `OLED_POWER_ON_SETTLE_MS` after powering the
  display before starting it. The wait didn't hold the display back, so a panel that
  powered up slowly could stay dark for the whole wake.
- Half-degree setpoints show as `21.5°`, rain in inches shows two decimals (`0.04 in`),
  and the text fonts now include accented letters, `²` `³` `µ`, curly quotes, dashes, `•`
  and `€`.
- `Circle` turns a light or fan on at its last brightness or speed instead of 100%.
- Locks, covers, switches and the alarm show a failure when the device doesn't respond.
  Home Assistant doesn't report a device's own errors back, so `LOCKING...` used to just
  disappear; the remote now watches the device's state from the moment it sends the
  command (15 s for locks, 20 s for covers, 5 s for switches).
- Automations show `TRIGGERED` right after the request instead of when the whole
  automation has finished, which could be minutes later, on another item.
- Entities Home Assistant reports as unavailable show `UNAVAILABLE` instead of `SYNCING`
  or the last state they had, and the action buttons say so instead of sending commands.
- Weather high, low and precipitation work again. Home Assistant removed the weather
  `forecast` attribute in 2024.3; the remote now asks `weather.get_forecasts` once per
  wake.
- OTA updates are now encrypted and authenticated with the API encryption key, and
  `ota_password` is no longer used; ESPHome 2026.9.0 or newer is required. If the remote
  runs firmware built with an older ESPHome, the first upload stops with "did not offer
  encryption": flash once over USB, or for that one upload put `password:
  !secret ota_password` back in place of `encryption:` under `ota:`.
- Added `tools/ui_preview/preview.py`, which renders every screen on your computer
  using the real renderer, ESPHome's display code and the project fonts.
- CI now builds the firmware with ESPHome 2026.9.1 (was 2026.7.3).
- CI also renders every screen and stress-tests the renderer with hostile input under
  AddressSanitizer and UndefinedBehaviorSanitizer (`preview.py --stress`), and attaches
  the screens to each run.
- Dependabot opens a pull request for each new ESPHome release and GitHub Actions
  update, so CI builds every PCB revision against it before it is merged.
- `prepare_ci_config.py` refuses to overwrite an existing `secrets.yaml` or
  `local_entities.h` outside CI unless given `--force`.
- Release notes no longer say firmware is attached. Releases include none, because each
  image carries its owner's Wi-Fi credentials and API key.
- Releases: a `VERSION` set by hand newer than every release is published as is (the
  automatic release always bumped it); otherwise the version is bumped past the latest
  tag, so a release can't reuse one. CI runs (pull requests, the weekly build) no longer
  cancel a release halfway, release notes list commits since the previous release on
  `main`, and rotating the changelog leaves a fresh `## Unreleased` heading.
- The Home Assistant notifications package keeps the newest 16 notifications, each cut
  to 120 characters. A long feed went past the remote's 8 KB limit and showed nothing.
  Notifications whose id contains `|` are left out, since they broke dismissing.
- CI uploads only the firmware it just built, and the UI preview tool copes with
  `esphome` installs whose launcher isn't a plain Python shebang.

## 3.11 — 2026-10-02
- The OLED is now put into its sleep mode before deep sleep instead of only being
  blanked. On rev1/rev2, which have no OLED power switch, the panel previously stayed
  active through sleep and kept draining the battery.
- Deep sleep forced by `DEEP_SLEEP_DURATION` now also turns the display off (and on
  rev3.1 cuts its power). It previously skipped the power-off sequence and left the
  last screen lit for the whole sleep.

## 3.10 — 2026-10-02
- Fixed buttons registering extra presses, which made Previous/Next skip items. Every
  button is now debounced, so contact bounce on press or release, or a brief dropout
  while a button is held, no longer counts as another tap. The new `BUTTON_DEBOUNCE_MS`
  setting (default 30 ms) controls the window; the wake button keeps
  `WAKE_BUTTON_DEBOUNCE_MS` but now also ignores a brief dropout while held.

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
