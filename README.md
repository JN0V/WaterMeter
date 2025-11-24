# WaterMeter v1.0.0

ESP32-based water meter pulse counter with Home Assistant integration, powered by [DomoticsCore](https://github.com/JN0V/DomoticsCore).

## Overview

This project turns any pulse-output water meter into a smart IoT device. It counts pulses, tracks consumption (Total, Daily, Yearly), and integrates seamlessly with Home Assistant's Energy Dashboard.

**Key Features:**
- **Advanced Pulse Logic:** High-State Stability check eliminates bounce/double-counting (ideal for slow flow).
- **Isolation Circuit:** MOSFET-based isolation prevents interference with existing meter readers.
- **Data Safety:** Auto-saves to NVS memory every 30s; auto-recovers after reboot.
- **Home Assistant:** Zero-config auto-discovery (MQTT). Supports Energy Dashboard natively (state_class: total_increasing).
- **Web Interface:** Configure network, MQTT, and view real-time stats via browser.
- **Automated Resets:** Daily (midnight) and Yearly (Jan 1st) counters reset automatically via NTP.

## Compatible Hardware

### Sensors
Compatible with any **Reed Switch (Dry Contact)** pulse output sensor.
- **Standard:** 2-wire "Pulse Probe" for water meters (e.g., Gianola, Itron Cyble Sensor 2-wire).
- **Generic:** Any generic magnetic reed switch.
- **Signal:** Connects to GPIO via the isolation circuit (see Wiring).

### Controller
- **ESP32 Development Board** (ESP32-WROOM-32 or similar).
- **Power:** 5V Micro-USB or VIN.

### Isolation Circuit (Required)
To prevent interference and protect the ESP32, we use a MOSFET buffer:
- **MOSFET:** IRLZ24N, IRLZ34N, or IRLZ44N (Logic-Level N-Channel)
- **Resistors:** 2x 10kΩ
- **Capacitor:** 1x 100nF (Ceramic)
- **See [docs/WIRING_GUIDE.md](docs/WIRING_GUIDE.md) for the complete schematic.**

## Quick Start

1. **Build the Circuit:** Follow the [Wiring Guide](docs/WIRING_GUIDE.md).
2. **Flash Firmware:**
   ```bash
   pio run -t upload
   ```
3. **Connect:**
   - Connect to WiFi AP: `WaterMeter-ESP32-XXXX`
   - Open `http://192.168.4.1`
   - Configure WiFi and MQTT settings.
4. **Home Assistant:**
   - Enable MQTT integration in HA.
   - The device will auto-discover 12 entities.
   - Add `total_liters` or `total_volume` to your **Energy Dashboard** (Water section).

## Documentation

- **[Wiring Guide](docs/WIRING_GUIDE.md):** Schematics and breadboard assembly.
- **[Breadboard Layout](docs/BREADBOARD_LAYOUT.md):** Visual guide for assembly.
- **[Testing Guide](docs/TESTING_GUIDE.md):** Verification and troubleshooting.
- **[Pulse Logic](docs/technical/PULSE_LOGIC.md):** Technical details on the anti-bounce algorithm.
- **[Architecture](docs/ARCHITECTURE.md):** Software design and component interaction.

## Pinout

| Pin | Function | Notes |
|-----|----------|-------|
| **GPIO34** | Pulse Input | Input-only pin. Requires external pull-up (see Wiring). |
| **GPIO32** | Status LED | Flashes on valid pulse. High-Z when ESP32 is off. |
| **GPIO2** | System LED | DomoticsCore status (WiFi/MQTT). |

## Requirements

- **PlatformIO** (VSCode extension recommended)
- **DomoticsCore v1.2.2+** (Automatically installed by PlatformIO)
- **MQTT Broker** (Mosquitto, EMQX, etc.)

## License

MIT License. See LICENSE file.
