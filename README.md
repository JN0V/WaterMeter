# WaterMeter v1.0.0

ESP32-based water meter pulse counter with Home Assistant integration.

**Powered by DomoticsCore v1.2.1** - Full-stack IoT framework with EventBus architecture.

## Features

- **Pulse counting** with boot protection (3s initialization delay)
- **MOSFET isolation** (IRLZ24N/34N/44N) - no interference with main system
- **Multi-period tracking** - Total, daily, yearly counters
- **Auto-resets** - Daily (midnight) and yearly (Jan 1st) via NTP
- **NVS storage** - Persistent data with auto-save (30s)
- **WebUI** - Configuration and monitoring on port 80
- **Home Assistant** - Auto-discovery via MQTT (12 entities)
- **Telnet console** - Remote access on port 23
- **OTA updates** - Secure firmware updates
- **LED indicators** - System status and pulse feedback

## Hardware

- ESP32 dev board
- MOSFET IRLZ24N/34N/44N (N-channel logic-level)
- 2x 10kΩ resistors
- 1x 100nF capacitor
- Magnetic pulse sensor (3.5V HIGH / 0.2V LOW)

**See [docs/WIRING_GUIDE.md](docs/WIRING_GUIDE.md) for complete circuit diagram**

## Quick Start

1. **Wire hardware:** Follow [docs/WIRING_GUIDE.md](docs/WIRING_GUIDE.md)
2. **Flash:** `pio run -t upload`
3. **Configure:** Connect to AP `WaterMeter-ESP32-XXXX` at http://192.168.4.1
4. **Test:** Follow [docs/TESTING_GUIDE.md](docs/TESTING_GUIDE.md)

## Documentation

- **[docs/README.md](docs/README.md)** - Complete documentation index
- **[docs/WIRING_GUIDE.md](docs/WIRING_GUIDE.md)** - Hardware wiring and MOSFET circuit
- **[docs/BREADBOARD_LAYOUT.md](docs/BREADBOARD_LAYOUT.md)** - Visual breadboard layout (color-coded)
- **[docs/TESTING_GUIDE.md](docs/TESTING_GUIDE.md)** - Testing procedures and troubleshooting

## Pinout

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO34 | Pulse input | Via MOSFET buffer |
| GPIO32 | Status LED | High-Z when off |
| GPIO2 | System LED | DomoticsCore status |

## Home Assistant

**12 auto-discovered entities:**
- 7 water sensors (total/daily/yearly in m³ and L)
- 2 system sensors (WiFi, uptime)
- 3 buttons (reset daily, reset yearly, restart)

**Setup:**
1. Configure MQTT broker in WebUI
2. Entities appear automatically in HA
3. No manual configuration needed

## Build

```bash
pio run -t upload      # Flash firmware
pio device monitor     # View serial output
```

## Console Commands

Telnet port 23:
- `water` - Show consumption
- `reset_daily` - Reset daily counter
- `reset_yearly` - Reset yearly counter

## Requirements

- PlatformIO
- DomoticsCore v1.2.1 (auto-installed)
- ESP32 dev board

## License

Open source. See LICENSE file.

---

**Full documentation:** [docs/README.md](docs/README.md)
