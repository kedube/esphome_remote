# ESPHome OLED Remote

ESPHome configuration for a handheld ESP32 OLED remote that controls Home Assistant entities from a compact button-based UI.

The current configuration supports these modes:

- Lights
- Switches
- Climate
- Humidifiers
- Fans
- Shades / covers
- Locks
- Media players
- Sensors
- Automations / scripts / scenes
- Alarms
- Notifications
- Weather
- Info

## Gallery

![Remote photo 1](images/remote_4.jpeg)

## UI Screenshots

| ![UI 1](images/remote_UI-1.png) | ![UI 2](images/remote_UI-2.png) | ![UI 3](images/remote_UI-3.png) | ![UI 4](images/remote_UI-4.png) | ![UI 5](images/remote_UI-5.png) |
| :---: | :---: | :---: | :---: | :---: |
| ![UI 6](images/remote_UI-6.png) | ![UI 7](images/remote_UI-7.png) | ![UI 8](images/remote_UI-8.png) | ![UI 9](images/remote_UI-9.png) | ![UI 10](images/remote_UI-10.png) | 
| ![UI 11](images/remote_UI-11.png) | ![UI 12](images/remote_UI-12.png) | ![UI 13](images/remote_UI-13.png) | ![UI 14](images/remote_UI-14.png) | ![UI 15](images/remote_UI-15.png) |
| ![UI 16](images/remote_UI-16.png) | ![UI 17](images/remote_UI-17.png) | ![UI 18](images/remote_UI-18.png) | ![UI 19](images/remote_UI-19.png) | ![UI 20](images/remote_UI-20.png) |

## Repo Layout

```text
esphome_remote/
├── README.md
├── images/
│   ├── remote_*.jpeg
│   └── remote_UI-*.png
└── src/
    ├── entity_helpers.h
    ├── framebuffer_web_debug.h
    ├── fonts/
    │   └── arial-bold.ttf
    ├── local_entities-example.h
    ├── oled_remote.yaml
    ├── pcb_proto.yaml
    ├── pcb_rev31.yaml
    └── secrets-example.yaml
```

## What Each File Does

- `src/oled_remote.yaml`
  Main ESPHome configuration, display logic, button handling, sleep behavior, and Home Assistant actions.

- `src/entity_helpers.h`
  Shared C++ helpers for entity metadata, mode navigation, state tracking, notification parsing, and weather/media helpers.

- `src/local_entities.h`
  Your private Home Assistant entity list. This file is not committed and is ignored by Git.

- `src/local_entities-example.h`
  Example entity definitions you can copy and customize.

- `src/framebuffer_web_debug.h`
  Optional debug-only web endpoint that exports the current OLED framebuffer as a downloadable PBM image.

- `src/pcb_proto.yaml`
  Hardware package for the prototype / older board layout.

- `src/pcb_rev31.yaml`
  Hardware package for PCB revision 3.1, including OLED power control and battery monitoring.

- `src/secrets-example.yaml`
  Example ESPHome secrets file.

## Quick Start

1. Clone the repo.

```bash
git clone https://github.com/kedube/esphome_remote
cd esphome_remote
```

2. Copy the example secrets file.

```bash
cp src/secrets-example.yaml src/secrets.yaml
```

3. Fill in your Wi-Fi and ESPHome credentials in `src/secrets.yaml`.

```yaml
wifi_ssid: "YourWiFiName"
wifi_password: "YourWiFiPassword"
encryption_key: "YourESPHomeAPIKey"
ota_password: "YourOTAPassword"
```

4. Copy the example entity list.

```bash
cp src/local_entities-example.h src/local_entities.h
```

5. Edit `src/local_entities.h` so it matches your Home Assistant setup.

6. Select the correct hardware package in `src/oled_remote.yaml`.

```yaml
packages:
  select_pcb: !include
    file: pcb_proto.yaml
    # file: pcb_rev31.yaml
```

7. Validate the configuration.

```bash
esphome config src/oled_remote.yaml
```

8. Build and flash the remote.

```bash
esphome run src/oled_remote.yaml
```

## Optional Framebuffer Download Debugging

When you want a clean UI capture from the SH1106 without photographing the screen, enable the framebuffer debug flag and the ESPHome web server together. The debug flag adds a framebuffer download endpoint, and the `web_server:` block exposes it over HTTP.

Temporary CLI override:

```bash
esphome -s FRAMEBUFFER_WEB_DEBUG 1 config src/oled_remote.yaml
esphome -s FRAMEBUFFER_WEB_DEBUG 1 run src/oled_remote.yaml
```

In `src/oled_remote.yaml`, set the debug substitution near the top:

```yaml
substitutions:
  FRAMEBUFFER_WEB_DEBUG: "1"
```

Then uncomment the web server section:

```yaml
web_server:
  port: 80
```

The file already includes a commented reminder near that section:

```yaml
# Uncomment to enable framebuffer web debugging on port 80.
# Make sure to also set REMOTE_FRAMEBUFFER_WEB_DEBUG to 1 in the substitutions section above.
#web_server:
#  port: 80
```

After flashing, browse to:

- `http://<device-ip>/debug/framebuffer.pbm`

Notes:

- The default build keeps the framebuffer dump endpoint off.
- If `FRAMEBUFFER_WEB_DEBUG` is enabled but `web_server:` stays commented out, the PBM download URL will not be reachable.
- The download is a 1-bit PBM image generated from the live OLED framebuffer.
- This is intended for debugging and README screenshots, not normal day-to-day use.

## Local Entity Configuration

`src/local_entities.h` defines the Home Assistant entities shown in each mode. The example file includes all supported lists:

- `LIGHT_LIST`
- `SWITCH_LIST`
- `CLIMATE_LIST`
- `HUMIDIFIER_LIST`
- `FAN_LIST`
- `COVER_LIST`
- `LOCK_LIST`
- `MEDIA_PLAYER_LIST`
- `SENSOR_LIST`
- `AUTOMATION_LIST`
- `ALARM_LIST`
- `WEATHER_LIST`

You can leave any of these lists empty:

- Empty lists compile cleanly.
- Modes with no configured entities are automatically hidden from the UI menu.
- You do not need to remove unused mode code by hand.

Example:

```cpp
static const LightEntity LIGHT_LIST[] = {
    {"Living Room Lamp", "light.living_room_lamp"},
};

static const MediaEntity MEDIA_PLAYER_LIST[] = {
    {"Bedroom TV", "media_player.bedroom_tv"},
    {"Receiver", "media_player.receiver", "Apple TV|PlayStation|Switch"},
};
```

`MEDIA_PLAYER_LIST` optionally supports a third field for a pipe-delimited fallback source list. That is useful when the device does not expose usable `source_list` data through Home Assistant.

Minimal empty-list example:

```cpp
static const AlarmEntity ALARM_LIST[] = {};
static const WeatherEntity WEATHER_LIST[] = {};
```

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
- `weather.*`

Notifications are read from `sensor.persistent_notifications` by default.

## UI Notes

- Lock, cover, and automation actions use long-press protection.
- The remote restores the previously selected mode and item after wake/reboot, including fan and humidifier selections.
- Modes with empty entity lists are skipped automatically when cycling through the menu.
- Media mode supports device-specific behavior for TVs, receivers, and speakers.
- Humidifier mode supports target humidity control and mode cycling.
- Notifications are shown from a Home Assistant sensor attribute feed.
- Info mode currently shows time/date and the configured firmware version.

## Important Settings

These substitutions live near the top of `src/oled_remote.yaml`:

- `TEMPERATURE_UNIT`
  Set to `"F"` or `"C"` to match your Home Assistant climate values.

- `SLEEP_DURATION`
  Idle time before the remote goes to sleep.

- `DEEP_SLEEP_DURATION`
  Sleep duration for deep sleep wake cycles.

- `LONG_PRESS_DURATION_MS`
  Hold time for protected actions.

- `LOW_BATTERY_VOLTAGE`
  Battery warning threshold when using hardware with battery monitoring.

- `PIN_*`
  Button, wake, and I2C pin assignments for the selected board.

## Hardware Notes

The configuration is built around:

- ESP32 development board
- SH1106 128x64 OLED over I2C
- Physical buttons for navigation and actions

Use the package include to choose the board revision:

- `src/pcb_proto.yaml`
  Basic prototype mapping

- `src/pcb_rev31.yaml`
  Revision 3.1 mapping with OLED power control, ADC battery voltage, and battery percentage reporting

## Troubleshooting

### ESPHome validation fails because `local_entities.h` is missing

Create it from the example file:

```bash
cp src/local_entities-example.h src/local_entities.h
```

### Wi-Fi will not connect

- Verify the values in `src/secrets.yaml`
- Confirm the remote can use your 2.4 GHz network
- Check signal strength near the device

### Home Assistant API will not connect

- Verify the ESPHome encryption key
- Confirm the device can reach Home Assistant on the same network
- Run:

```bash
esphome config src/oled_remote.yaml
```

### The wrong entities appear on screen

- Check `src/local_entities.h`
- Make sure each entity exists in Home Assistant
- Keep entity domains matched to the correct list type

### A mode is missing from the menu

- Check whether the corresponding list in `src/local_entities.h` is empty
- Modes are hidden automatically when their configured entity list has no items
- Re-run `esphome config src/oled_remote.yaml` after editing your lists

### Battery warnings never appear

- Make sure `pcb_rev31.yaml` is selected
- Confirm your hardware actually has battery monitoring wired up
- Adjust `LOW_BATTERY_VOLTAGE` if needed for your battery chemistry and calibration

## Privacy

Do not publish your real `src/local_entities.h` or `src/secrets.yaml`. The repo includes `src/local_entities-example.h` and `src/secrets-example.yaml` specifically so you can share the project safely.

## Related Links

- [ESPHome documentation](https://esphome.io/)
- [Project article](https://tech.lugowski.dev/smart-oled-remote-for-home-assistant/)
- [MakerWorld case files](https://makerworld.com/en/models/1902607-home-assistant-esphome-remote-with-oled-display#profileId-2039332)

## License

See [LICENSE](LICENSE).
