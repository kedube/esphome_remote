# ESPHome Remote Control

Replacement firmware for [Pawel Lugowski's ESPHome OLED Remote Control](https://tech.lugowski.dev/guides/smart-oled-remote-esphome/). The hardware is built around an ESP32 Lolin32 WROOM (WIFI + Bluetooth) board, a 1.3-inch SH1106 128x64 OLED display, and physical buttons that provide a compact, battery-friendly UI for controlling Home Assistant entities directly from the handheld remote. 

The firmware has been entirely rewritten from scratch based on a newly designed codebase and architecture. It is designed to let you cycle through Home Assistant entities directly from the remote without needing a touchscreen or a phone. The remote now uses mixed-entity favorite lists as the primary navigation model, while still supporting controls for lights, switches, climate devices, humidifiers, fans, covers, locks, media players, sensors, automations, alarms, weather, notifications, and info screens.

## Gallery

![Remote photo 1](images/remote_1.jpeg)

## Remote UI Screenshots

| ![UI 1](images/remote_UI-1.png) | ![UI 2](images/remote_UI-2.png) | ![UI 3](images/remote_UI-3.png) | ![UI 4](images/remote_UI-4.png) | ![UI 5](images/remote_UI-5.png) |
| :---: | :---: | :---: | :---: | :---: |
| ![UI 6](images/remote_UI-6.png) | ![UI 7](images/remote_UI-7.png) | ![UI 8](images/remote_UI-8.png) | ![UI 9](images/remote_UI-9.png) | ![UI 10](images/remote_UI-10.png) <tr></tr> | 
| ![UI 11](images/remote_UI-11.png) | ![UI 12](images/remote_UI-12.png) | ![UI 13](images/remote_UI-13.png) | ![UI 14](images/remote_UI-14.png) | ![UI 15](images/remote_UI-15.png) <tr></tr> |
| ![UI 16](images/remote_UI-16.png) | ![UI 17](images/remote_UI-17.png) | ![UI 18](images/remote_UI-18.png) | ![UI 19](images/remote_UI-19.png) | ![UI 20](images/remote_UI-20.png) <tr></tr> |

## Features

- Button-driven UI optimized for a 128x64 monochrome OLED
- Deep sleep support for battery-powered remotes
- Multiple board package options for different PCB revisions
- Favorite-list navigation with mixed Home Assistant entity types in each list
- Automatic hiding of empty favorite lists and optional Notifications mode
- Persistent restore of the current favorite list, selected item per favorite list, and contrast
- Notification, weather, version, time/date, and network info screens
- Optional framebuffer download endpoint for capturing clean UI screenshots

## Quick Start

If you just want to get the remote running:

1. Install ESPHome.
2. Copy [`esphome/examples/secrets-example.yaml`](esphome/examples/secrets-example.yaml) to [`esphome/secrets.yaml`](esphome/secrets.yaml) and fill in your Wi-Fi, OTA, and API credentials.
3. Copy [`esphome/examples/local_entities-example.h`](esphome/examples/local_entities-example.h) to [`esphome/local_entities.h`](esphome/local_entities.h) and define your favorite lists.
4. Open [`esphome/settings.yaml`](esphome/settings.yaml) and choose the correct PCB package.
5. Run `esphome run esphome/remote_control.yaml`.

The current release series is `2.7`.

## Navigation Model

- One or more user-defined favorite lists containing mixed entity types
- Optional Notifications screen after the favorite lists
- Info screen always available at the end of the menu

Each favorite list shows the list name in the header, the selected entry name in the main title row, and the current entity domain icon in the corners. The control surface adapts automatically to the selected entity type.

## Hardware

This configuration is built around:

- ESP32 Lolin32 WROOM (WIFI + Bluetooth) development board
- 1.3-inch SH1106 128x64 OLED display over I2C
- Remote PCB designed by Pawel Lugowski
- Physical navigation and action buttons
- 3D Printed Case and Buttons

Board-specific wiring is selected through the PCB package include in [`esphome/settings.yaml`](esphome/settings.yaml).

`esphome/remote_control.yaml` includes that shared settings file, while [`esphome/settings.yaml`](esphome/settings.yaml) contains the common substitutions, PCB package selection, and optional `web_server` block.

Please refer to the [Quick Start Guide](https://tech.lugowski.dev/smart-remote-kit/) for more details:

- `esphome/packages/pcb_rev1.yaml`
  Revision 1 board mapping (no OLED power control or battery monitoring)
- `esphome/packages/pcb_rev2.yaml`
  Revision 2 board mapping with battery monitoring (no OLED power control)
- `esphome/packages/pcb_rev31.yaml`
  Revision 3.1 mapping with OLED power control and battery monitoring

## Repo Layout

```text
esphome_remote/
├── LICENSE
├── README.md
├── assets/
│   └── fonts/
│       └── arial-bold.ttf
├── esphome/
│   ├── .gitignore
│   ├── examples/
│   │   ├── local_entities-example.h
│   │   └── secrets-example.yaml
│   ├── packages/
│   │   ├── pcb_rev1.yaml
│   │   ├── pcb_rev2.yaml
│   │   ├── pcb_rev31.yaml
│   │   ├── remote_actions_automation.yaml
│   │   ├── remote_actions_climate_media.yaml
│   │   ├── remote_actions_devices.yaml
│   │   ├── remote_actions_feedback.yaml
│   │   ├── remote_actions_security.yaml
│   │   ├── remote_button_action_scripts.yaml
│   │   ├── remote_button_press_scripts.yaml
│   │   ├── remote_display_core.yaml
│   │   ├── remote_display_runtime_globals.yaml
│   │   ├── remote_display_scripts.yaml
│   │   ├── remote_display_selection_globals.yaml
│   │   ├── remote_display_state_globals.yaml
│   │   ├── remote_fonts.yaml
│   │   ├── remote_inputs.yaml
│   │   ├── remote_runtime.yaml
│   │   ├── remote_ui_navigation_actions.yaml
│   │   ├── remote_ui_selection_scripts.yaml
│   │   └── remote_ui_setup_scripts.yaml
│   ├── local_entities.h
│   ├── remote_control.yaml
│   ├── secrets.yaml
│   └── settings.yaml
├── home_assistant/
│   └── remote_notifications.yaml
├── include/
│   ├── entity_helpers_common.h
│   ├── entity_helpers.h
│   ├── entity_helpers_requests.h
│   ├── entity_trackers.h
│   ├── framebuffer_web_debug.h
│   ├── local_entities.h
│   ├── remote_ui_bindings.h
│   ├── remote_ui_feedback.h
│   ├── remote_ui_input_logic.h
│   ├── remote_ui_logic.h
│   ├── remote_ui_renderer.h
│   ├── remote_ui_runtime.h
│   ├── remote_ui_sync.h
│   └── ui_state_helpers.h
├── images/
│   ├── remote_*.jpeg
│   └── remote_UI-*.png
└── src/
    ├── framebuffer_web_debug.cpp
    ├── remote_ui_feedback.cpp
    ├── remote_ui_input_logic.cpp
    ├── remote_ui_logic.cpp
    ├── remote_ui_renderer.cpp
    ├── remote_ui_runtime.cpp
    └── remote_ui_sync.cpp
```

## Important Files

- `esphome/remote_control.yaml`
  Main ESPHome entrypoint that pulls together shared packages, secrets, local entity definitions, fonts, and runtime logic.
- `esphome/settings.yaml`
  Shared configuration file for common substitutions, PCB selection, and optional web server settings.
- `include/entity_helpers.h`
  Compatibility shim that includes the tracker and request helper layers.
- `include/entity_trackers.h`
  Home Assistant tracker classes that subscribe to and cache entity state.
- `include/entity_helpers_common.h` and `include/entity_helpers_requests.h`
  Shared entity metadata/state helpers and the request/query layer built on top of the trackers.
- `esphome/local_entities.h`
  Your private Home Assistant entity definitions and favorite lists. This file is ignored by Git and lives next to `secrets.yaml` for a simpler compile workflow.
- `esphome/examples/local_entities-example.h`
  Example entity definitions and favorite lists you can copy and customize.
- `include/local_entities.h`
  Compatibility shim that forwards to `esphome/local_entities.h`.
- `esphome/packages/`
  Modular ESPHome packages for actions, button/input handling, runtime behavior, display globals, fonts, and UI scripts.
- `src/framebuffer_web_debug.cpp` and `include/framebuffer_web_debug.h`
  Optional debug-only PBM framebuffer export for screenshot capture.
- `home_assistant/remote_notifications.yaml`
  Optional Home Assistant template sensor bridge for the Notifications mode.

## Architecture Notes

- `include/entity_helpers_common.h`
  Shared metadata, favorite-list plumbing, internal domain indexing, selection helpers, and configuration validation.
- `include/entity_trackers.h`
  Home Assistant tracker classes that subscribe to and cache entity state.
- `include/entity_helpers_requests.h`
  Thin request/sync helpers that bridge tracker state into the UI logic.
- `include/remote_ui_bindings.h`
  Shared UI binding helpers used to build reset/sync state objects without duplicating YAML wiring blocks.
- `include/ui_state_helpers.h`
  Persistent UI save/restore helpers for favorite-list menu state.
- `esphome/remote_control.yaml`
  Top-level composition file that imports the modular ESPHome packages and C++ helpers.

The `esphome/packages/` folder is currently split by responsibility:

- `remote_actions_*.yaml`
  Entity actions and feedback flows grouped by domain.
- `remote_button_*.yaml`
  Button press handling and action wrapper scripts.
- `remote_display_*.yaml`
  Display hardware, UI globals, and the render entry script.
- `remote_fonts.yaml`
  Font definitions, including the weather icon font.
- `remote_ui_*.yaml`
  UI setup, selection, and navigation scripts.
- `remote_inputs.yaml` and `remote_runtime.yaml`
  Physical input bindings and the runtime loop.

At startup the remote validates the configured favorite entries and logs warnings for missing names, missing entity IDs, or unsupported entity domains.

## 1. Install ESPHome

Follow the official directions on the [ESPHome website](https://esphome.io/guides/installing_esphome/).

If you're using MacOS, the easiest way to install is via [Homebrew](https://brew.sh/) by running this command in a MacOS terminal window:
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Now install ESPHome:
```bash
brew install esphome
```

## 2. Clone The Repository

```bash
git clone https://github.com/kedube/esphome_remote
cd esphome_remote
```

## 3. Create Your Secrets File

Copy the example secrets file:

```bash
cp esphome/examples/secrets-example.yaml esphome/secrets.yaml
```

Then fill in your Wi-Fi, OTA, and API encryption details:

```yaml
wifi_ssid: "YourWiFiName"
wifi_password: "YourWiFiPassword"
encryption_key: "YourESPHomeAPIKey"
ota_password: "YourOTAPassword"
```

## 4. Create Your Favorite Lists

Copy the example favorite-list file:

```bash
cp esphome/examples/local_entities-example.h esphome/local_entities.h
```

Edit `esphome/local_entities.h` so it matches your Home Assistant setup.

The file now only needs favorite lists. Each `FavoriteEntity` entry provides a display name and a Home Assistant `entity_id`, and the remote infers the entity type from the `entity_id` prefix such as `light.`, `switch.`, `climate.`, `weather.`, and so on.

You can define as many favorite lists as you want. Empty favorite lists compile cleanly and are skipped automatically in the menu.

_Example:_

```cpp
static const FavoriteEntity MAIN_FAVORITES[] = {
  {"Living Room Lamp", "light.living_room_lamp"},
  {"Bedroom TV", "media_player.bedroom_tv"},
  {"Main Thermostat", "climate.main_thermostat"},
  {"Front Door", "lock.front_door"},
};

static const FavoriteList FAVORITE_LISTS[] = {
  make_favorite_list("MAIN", MAIN_FAVORITES),
};
```

Minimal multi-list example:

```cpp
static const FavoriteEntity UPSTAIRS_FAVORITES[] = {
  {"Hallway Thermostat", "climate.hallway_thermostat"},
  {"Bedroom Fan", "fan.bedroom_fan"},
};

static const FavoriteEntity OUTDOOR_FAVORITES[] = {};

static const FavoriteList FAVORITE_LISTS[] = {
  make_favorite_list("UPSTAIRS", UPSTAIRS_FAVORITES),
  make_favorite_list("OUTDOOR", OUTDOOR_FAVORITES),
};
```

Notifications are configured in the same file with optional feed defines:

```cpp
#define NOTIFICATION_FEED_ENTITY "sensor.remote_notifications"
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#define NOTIFICATION_FEED_SEPARATOR "||"
```

Notes:

- Set `NOTIFICATION_FEED_ENTITY` to an empty string to hide Notifications completely.
- `NOTIFICATION_FEED_ENTITY` is the Home Assistant entity the remote reads from.
- `NOTIFICATION_FEED_ATTRIBUTE` is the attribute on that entity containing the notification payload.
- `NOTIFICATION_FEED_IDS_ATTRIBUTE` is the attribute on that entity containing notification IDs for dismiss actions.
- `NOTIFICATION_FEED_SEPARATOR` is used when multiple notifications are packed into one string.

Since Home Assistant no longer exposes a ready-made `sensor.persistent_notifications`, the included template sensor must be added to Home Assistant to recreate the feed the remote expects.

Copy [`home_assistant/remote_notifications.yaml`](home_assistant/remote_notifications.yaml) into your Home Assistant `template:` configuration, or include it as a package. It publishes:

- `sensor.remote_notifications`
- state = current notification count
- attribute `messages` = all active persistent notifications packed into one `||`-separated string
- attribute `ids` = matching persistent notification IDs packed in the same order

This bridge is event-driven. It listens for Home Assistant `persistent_notification` updates, stores the active notification list in the template sensor’s own attributes, and exposes a `messages` attribute that the remote can read. Each item is emitted as `Title: Message`, with newlines flattened to spaces so the remote can render them cleanly.

In Notifications mode, pressing the circle or play/pause action button dismisses the currently selected persistent notification. The display shows `DISMISSED` for 3 seconds, then refreshes and advances to the next remaining notification.

## 5. Configure `settings.yaml`

Open `esphome/settings.yaml` and update the shared settings for your remote. This file is intended to be the main place for device-level customization.

At a minimum, choose the board package that matches your remote hardware PCB:

```yaml
packages:
  select_pcb: !include
    #file: packages/pcb_rev1.yaml
    #file: packages/pcb_rev2.yaml
    file: packages/pcb_rev31.yaml
```

This file also contains the most common substitutions you may want to change:

| Setting | Purpose |
| --- | --- |
| `BOARD` | ESPHome board definition, currently `esp32dev`. |
| `DEVICE_NAME` | Network name used by ESPHome and OTA. |
| `FRIENDLY_NAME` | Human-readable device name shown in Home Assistant. |
| `NOTIFICATION_FEED_MAX_ITEMS` | Maximum number of notification messages cached and exposed in Notifications mode. |
| `MAX_PERSISTED_FAVORITE_LISTS` | Maximum number of favorite lists stored in persisted UI state. This must be at least as large as your configured favorite list count. |
| `TEMPERATURE_UNIT` | Set to `"F"` or `"C"` to match your Home Assistant climate values. |
| `SLEEP_DURATION` | Idle time before the remote sleeps. |
| `DEEP_SLEEP_DURATION` | Duration of deep sleep. |
| `LONG_PRESS_DURATION_MS` | Hold time for protected actions. |
| `EXTENDED_HOLD_DURATION_MS` | Shared hold time for long protected actions that use the extended timer, including the Settings button alarm trigger and wake-button reboot. |
| `WAKE_BUTTON_DEBOUNCE_MS` | Debounce time for the wake/power button press and release handling. |
| `NAVIGATION_SYNC_DELAY_MS` | Short quiet period after navigation before subscription-backed state sync resumes. |
| `REBOOT_MESSAGE_DURATION_MS` | How long the `REBOOTING...` message stays on screen before the remote restarts. |
| `ALARM_CODE` | Optional alarm code. Leave it empty or replace it with `!secret alarm_code`. |
| `LOW_BATTERY_VOLTAGE` | Battery warning threshold for battery-monitoring boards. |
| `FRAMEBUFFER_WEB_DEBUG` | Set to `"1"` only when using the optional framebuffer download endpoint. |

Notes:

- `NOTIFICATION_FEED_MAX_ITEMS` and `MAX_PERSISTED_FAVORITE_LISTS` are compile-time capacity limits. Changing them requires recompiling the firmware and may increase memory usage.
- The timing substitutions such as `SLEEP_DURATION`, `LONG_PRESS_DURATION_MS`, `EXTENDED_HOLD_DURATION_MS`, `WAKE_BUTTON_DEBOUNCE_MS`, `NAVIGATION_SYNC_DELAY_MS`, and `REBOOT_MESSAGE_DURATION_MS` control runtime behavior and are the safest settings to tune first.
- Safe starting points:
  `NOTIFICATION_FEED_MAX_ITEMS: "16"`, `MAX_PERSISTED_FAVORITE_LISTS: "16"`, `WAKE_BUTTON_DEBOUNCE_MS: "30"`, `NAVIGATION_SYNC_DELAY_MS: "250"`, `REBOOT_MESSAGE_DURATION_MS: "2000"`.

`esphome/remote_control.yaml` still contains the board-specific `PIN_*` substitutions that are provided by the selected PCB package.

If your alarm integration requires a code, set it in `esphome/settings.yaml` like this:

```yaml
substitutions:
  ALARM_CODE: !secret alarm_code
```

Then add the value to `esphome/secrets.yaml`:

```yaml
alarm_code: "1234"
```

## 6. Validate The Configuration

```bash
esphome config esphome/remote_control.yaml
```

## 7. Build And Flash

```bash
esphome run esphome/remote_control.yaml
```

You must connect the remote via USB to your computer in order to perform the first flash. It will prompt you after a successful build for where to upload the code. After the first flash, future updates can be done over OTA.

```text
INFO Build Info: config_hash=0x694d2e36 build_time_str=2026-04-01 14:02:17 -0400
INFO Successfully compiled program.
Found multiple options for uploading, please choose one:
  [1] /dev/cu.usbserial-8320 (USB Serial)
  [2] Over The Air (esphome-remote.local)
(number):
```

## Optional Framebuffer Download Debugging

If you want clean screenshots of the OLED UI, the project can expose the current framebuffer as a downloadable PBM image.

Enable the framebuffer debug flag in [`esphome/settings.yaml`](esphome/settings.yaml):

```yaml
substitutions:
  FRAMEBUFFER_WEB_DEBUG: "1"
```

Then, uncomment the web server section in the same file:

```yaml
web_server:
  port: 80
```

You can also use a CLI substitution override:

```bash
esphome -s FRAMEBUFFER_WEB_DEBUG 1 config esphome/remote_control.yaml
esphome -s FRAMEBUFFER_WEB_DEBUG 1 run esphome/remote_control.yaml
```

After flashing, browse to:

- `http://esphome-remote.local/debug/framebuffer.pbm`

Notes:

- The framebuffer download endpoint is off by default.
- If `FRAMEBUFFER_WEB_DEBUG` is enabled but `web_server:` remains commented out, the URL will not be reachable.
- The PBM image is generated from the live OLED framebuffer.
- This is mainly intended for debugging and README screenshots.

## Button Guide

The remote is designed around nine physical inputs:

| Button | Default behavior |
| --- | --- |
| Wake / Power | Short-press and release puts the remote to sleep. Hold for `EXTENDED_HOLD_DURATION_MS` to reboot. |
| Mode | Cycles to the next top-level mode or favorite list. |
| Previous | Selects the previous item in the current mode. |
| Next | Selects the next item in the current mode. |
| Settings | Cycles through the available detail/settings views for the current item. In alarm mode, hold for `EXTENDED_HOLD_DURATION_MS` to trigger the alarm action. |
| Minus | Decreases the current adjustable setting. In Weather mode, it cycles backward through detail views. |
| Plus | Increases the current adjustable setting. In Weather mode, it cycles forward through detail views. |
| Circle | Positive or activate action in most modes: turn on, open, lock, play/pause, run, arm, or dismiss. |
| Square | Negative or deactivate action in most modes: turn off, close, unlock, stop, or disarm. |

Common usage pattern:

- Use `Mode` to move between favorite lists, Notifications, and Info.
- Use `Previous` and `Next` to choose an item.
- Use `Settings` to pick which detail or control you want to adjust.
- Use `Plus` and `Minus` to change the selected value or browse weather details.
- Use `Circle` and `Square` for the main action on the current item.

Long-press protection:

- Locks, covers, and automations require holding the action button for `LONG_PRESS_DURATION_MS`.
- Alarm arming and disarming also use long-press protection.
- Alarm trigger on the Settings button and wake-button reboot both use `EXTENDED_HOLD_DURATION_MS`.

## Mode Map

| Mode | Primary actions |
| --- | --- |
| Favorites: Lights | `Circle` on, `Square` off, `Settings` selects brightness/effect when available. |
| Favorites: Switches | `Circle` on, `Square` off. |
| Favorites: Climate | `Circle` on, `Square` off, `Settings` cycles target/fan/humidity/preset/state views. |
| Favorites: Humidifiers | `Circle` on, `Square` off, `Settings` cycles humidity/mode/action/state views. |
| Favorites: Fans | `Circle` on, `Square` off, `Settings` can expose speed/preset/oscillation/direction. |
| Favorites: Covers | `Circle` open, `Square` close. |
| Favorites: Locks | `Circle` lock, `Square` unlock. |
| Favorites: Media | `Circle` play/pause, `Square` stop, `Settings` cycles volume/source/shuffle/repeat/sound/state views. |
| Favorites: Water Heaters | `Circle` on, `Square` off, `Settings` cycles target/mode/away views. |
| Favorites: Automation / Script / Scene | `Circle` run. |
| Notifications | `Circle` dismiss selected notification. |
| Alarms | `Circle` arm, `Square` disarm, `Plus` / `Minus` change arm mode. |
| Weather | `Settings`, `Plus`, and `Minus` browse weather detail views. |
| Info | Read-only status screens for time/date, network, and version. |

Mode-specific action examples:

- Lights: `Circle` turns on, `Square` turns off, `Settings` can expose brightness and effect controls, and `Plus` / `Minus` adjust the selected setting.
- Switches: `Circle` turns on and `Square` turns off.
- Climate: `Circle` turns on, `Square` turns off, `Settings` cycles through target temperature, fan, humidity, preset, action, and state views when available.
- Humidifiers: `Circle` turns on, `Square` turns off, and `Settings` cycles humidity, mode, action, and state views.
- Fans: `Circle` turns on, `Square` turns off, and `Settings` can expose speed, preset, oscillation, and direction.
- Covers: `Circle` opens, `Square` closes, and long-press protection applies.
- Locks: `Circle` locks, `Square` unlocks, and long-press protection applies.
- Media: `Circle` play/pause, `Square` stop, and `Settings` can expose volume, source, shuffle, repeat, sound mode, and state.
- Notifications: `Circle` dismisses the selected notification.
- Alarms: `Circle` arms with the selected arm mode, `Square` disarms, and `Settings` cycles the alarm-state view. `Plus` and `Minus` change the selected arm mode.
- Weather: `Settings`, `Plus`, and `Minus` are used to browse condition, humidity, wind, pressure, precipitation, UV, dew point, apparent temperature, and high/low detail views.

## UI Notes

- The remote restores the previously selected favorite list and selected item after wake or reboot.
- Empty favorite lists are skipped automatically.
- Holding the wake/power button for `EXTENDED_HOLD_DURATION_MS` reboots the remote. The screen shows `HOLD TO REBOOT` while held, then `REBOOTING...` briefly before restart.
- Lock, cover, and automation actions use long-press protection.
- When a favorite entry resolves to a lock, circle locks and square unlocks. The remote shows temporary detail-line feedback such as `LOCKING...`, `UNLOCKING...`, `LOCKED`, `UNLOCKED`, `JAMMED`, `ALREADY LOCKED`, and `ALREADY UNLOCKED`.
- When a favorite entry resolves to a cover, circle opens and square closes. The remote shows temporary detail-line feedback such as `OPENING...`, `CLOSING...`, `OPENED`, `CLOSED`, and `OPEN xx%`.
- When a favorite entry resolves to an automation, script, or scene, the remote shows temporary feedback such as `TRIGGERING...`, `ACTIVATING...`, `RUNNING...`, `TRIGGERED`, `ACTIVATED`, `STARTED`, and `COMPLETED`.
- When a favorite entry resolves to an alarm, circle long-press arms using the currently selected arm mode when the panel is disarmed.
- When a favorite entry resolves to an alarm, square long-press disarms the panel.
- When a favorite entry resolves to an alarm, dimmer up and dimmer down cycle `away`, `home`, `night`, and `vacation` arm modes in the details line for 5 seconds.
- When a favorite entry resolves to an alarm, the Settings button must be held for `EXTENDED_HOLD_DURATION_MS` to call `alarm_trigger`. The details line shows `HOLD TO TRIGGER` while held.
- Alarm actions use temporary details-line feedback such as `ARMING...`, `DISARMING...`, `TRIGGERING...`, `ARMED HOME`, `DISARMED`, `TRIGGERED`, and `FAILED`-style responses when Home Assistant reports an error.
- Info mode includes Time & Date, Network, and Version screens.
- Notifications reads from `NOTIFICATION_FEED_ENTITY` in `esphome/local_entities.h`.

## Supported Home Assistant Entity Domains

- `light.*`
- `switch.*`
- `climate.*`
- `humidifier.*`
- `fan.*`
- `cover.*`
- `lock.*`
- `media_player.*`
- `sensor.*`
- `binary_sensor.*`
- `automation.*`
- `script.*`
- `scene.*`
- `alarm_control_panel.*`
- `water_heater.*`
- `weather.*`

## Important Settings

These substitutions in `esphome/settings.yaml` are the main things you may want to customize:

- `TEMPERATURE_UNIT`
  Set to `"F"` or `"C"` to match your Home Assistant climate values.
- `NOTIFICATION_FEED_MAX_ITEMS`
  Maximum number of notification items retained in the remote's notification feed cache.
- `MAX_PERSISTED_FAVORITE_LISTS`
  Maximum number of favorite lists that can be stored in the persisted UI state.
- `SLEEP_DURATION`
  Idle time before the remote sleeps.
- `DEEP_SLEEP_DURATION`
  Duration of deep sleep.
- `LONG_PRESS_DURATION_MS`
  Hold time for protected actions.
- `EXTENDED_HOLD_DURATION_MS`
  Shared hold time for the alarm trigger action on the Settings button and wake-button reboot.
- `WAKE_BUTTON_DEBOUNCE_MS`
  Debounce time for the wake/power button.
- `NAVIGATION_SYNC_DELAY_MS`
  Delay before the UI resumes sync after navigation.
- `REBOOT_MESSAGE_DURATION_MS`
  Duration of the `REBOOTING...` confirmation screen.
- `ALARM_CODE`
  Optional alarm code. This can stay empty or be set from `!secret alarm_code`.
- `LOW_BATTERY_VOLTAGE`
  Battery warning threshold for battery-monitoring boards.
- `PIN_*`
  Button, wake, and I2C pin assignments are defined by the selected board package.

## Troubleshooting

### `local_entities.h` is missing

Create it from the example file:

```bash
cp esphome/examples/local_entities-example.h esphome/local_entities.h
```

### `secrets.yaml` is missing

Create it from the example file:

```bash
cp esphome/examples/secrets-example.yaml esphome/secrets.yaml
```

### A favorite list does not appear in the menu

That usually means the corresponding favorite list is empty. Empty favorite lists are intentionally hidden.

### A Home Assistant entity does not respond

Ensure there are no typos in the entity name. You can check the list of entity names from __Home Assistant->Settings->Developer Tools->Template__. In the Template editor, use this example (replace 'light' with the entity domain you'd like to search):

```yaml
{% for e in states if 'light.' in e.entity_id %}
{{ e.entity_id }}
{% endfor %}
```

### Alarm arm or disarm actions fail

Check these items:

- Your alarm entity supports the requested service, such as `alarm_arm_home`, `alarm_arm_night`, or `alarm_disarm`.
- If your integration requires a code, `ALARM_CODE` is set in `esphome/settings.yaml` and `alarm_code` exists in `esphome/secrets.yaml`.
- If your integration does not require a code, leave `ALARM_CODE` empty.

### The framebuffer download URL does not work

Make sure both of these are true:

- `FRAMEBUFFER_WEB_DEBUG: "1"` is set in `esphome/settings.yaml`
- `web_server:` is uncommented in `esphome/settings.yaml`

### ESPHome compile or upload fails

Start with:

```bash
esphome config esphome/remote_control.yaml
```

If validation succeeds, retry with:

```bash
esphome run esphome/remote_control.yaml
```

## Related Links

- [ESPHome OLED remote control project article](https://tech.lugowski.dev/guides/smart-oled-remote-esphome/)
- [Etsy store for buying PCBs or Full Remotes](https://www.etsy.com/listing/4390635949/home-assistant-esphome-oled-remote)
- [MakerWorld case files](https://makerworld.com/en/models/1902607-home-assistant-esphome-remote-with-oled-display)
