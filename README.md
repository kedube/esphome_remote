# ESPHome Remote Configuration

This repository contains ESPHome YAML configuration files for remote devices and sensors, specifically designed for Home Assistant entity control using ESP32 boards with OLED displays. It supports the control of:
- Lights
- Climate (Thermostats)
- Music Players
- Triggering Automations, Scripts, or Scenes
- Weather Reports
- Info (Date/Version/Battery Details)

## Overview

ESPHome is a system to control your ESP8266/ESP32 by simple yet powerful configuration files and control them remotely through Home Assistant. This repository serves as a centralized location for managing ESPHome device configurations for remote control of Home Assistant devices.

## Repository Structure

```
esphome_remote/
├── README.md                                    # This file
├── imagees/*                                    # Pictures of the remote UI
└── devices/
    └── oled_remote/
        ├── oled_remote.yaml                     # Main multi-mode OLED remote functionality for Home Assistant
        ├── pcb_proto.yaml                       # Configuration for older PCBs without battery monitoring
        ├── pcb_rev32.yaml                       # Configuration for PCB v3.1 and newer with fixed voltage divider        
        ├── secrets-example.yaml                 # Example secrets file to copy and edit with your specific settings       
        └── ha_entities.h                        # C++ header for Home Assistant entity helpers and where you define your entities
```

## Quick Start Guide

### Prerequisites

- ESP32 development board (Lolin V1.0.0)
- Home Assistant instance with API access
- OLED display (SH1106 128x64)

### Setting Up a New Device

1. **Clone Repository**

```
git clone https://github.com/kedube/esphome_remote
cd esphome_remote/devices/oled_remote
```

2. **Configure settings** in your ESPHome secrets:
   - Copy `secrets-example.yaml` to `secrets.yaml` file in your ESPHome directory and configure settings:

     ```
     wifi_ssid: "YourWiFiName"
     wifi_password: "YourWiFiPassword"
     encryption_key: "YourAPIEncryptionKey"
     ota_password: "YourOTAPassword"
     ```

3. **Update Home Assistant entities** (if needed):
   - Edit `ha_entities.h` to match your Home Assistant entities
   - The default configuration includes numerous entities for several classification of devices: Lights, Climate, Music, Automations, & Weather

4. **Compile and upload** to your ESP32

```
esphome run oled_remote.yaml
```

### Important Notes

#### What to Customize

**Optional customization:**
- WiFi power settings (if signal issues occur)
- Deep sleep duration (default: 3 days)
- Idle timeout (default: 2 minutes)
- Display contrast settings
- Entity IDs in `ha_entities.h`

**Leave unchanged (for Lolin board):**
- `BOARD` setting
- All `PIN_*` configurations
- I2C frequency and settings
- Display model and configuration

## Hardware Setup

### Recommended Components

- **ESP32 Lolin** development board
- **SH1106 128x64 OLED** display (I2C)
- **Push buttons** (8 total for full functionality)

## Features

- **Multi-Light Control**: Control multiple types of entities from one device
- **Deep Sleep**: Battery-efficient operation with automatic sleep
- **OLED Display**: Real-time status display with custom fonts
- **Button Interface**: Physical buttons for all common operations
- **Home Assistant Integration**: Full API integration with encryption
- **Auto-sync**: Automatically syncs with Home Assistant entity states

## Troubleshooting

### Device won't connect to WiFi
- Verify WiFi credentials in `secrets.yaml`
- Check WiFi signal strength
- Try increasing `output_power` in the WiFi section

### Display not working
- Verify I2C pin connections (SDA: GPIO27, SCL: GPIO25)
- Check display model in configuration (SH1106 vs SSD1306)
- Verify display I2C address (default: 0x3C)

### Buttons not responding
- Check pin assignments match your hardware
- Verify pull-up resistor configuration
- Test with a multimeter for button continuity

### API connection fails
- Verify API key matches in both ESPHome and Home Assistant
- Check Home Assistant is accessible from ESP32 network
- Ensure firewall allows API connections

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

For issues and questions:
- Check the [ESPHome documentation](https://esphome.io/)
- [Full project details](https://tech.lugowski.dev/smart-oled-remote-for-home-assistant/)
- 3D Printed case available on [MakerWorld](https://makerworld.com/en/models/1902607-home-assistant-esphome-remote-with-oled-display#profileId-2039332)
- Open an issue in this repository
