# WaterMeter Documentation

## Overview

ESP32-based water meter pulse counter with Home Assistant integration using DomoticsCore framework.

## Documentation Structure

- **[WIRING_GUIDE.md](WIRING_GUIDE.md)** - Hardware wiring and MOSFET isolation circuit
- **[BREADBOARD_LAYOUT.md](BREADBOARD_LAYOUT.md)** - Visual breadboard layout with color-coded wiring
- **[TROUBLESHOOTING_ASSEMBLY.md](TROUBLESHOOTING_ASSEMBLY.md)** - Assembly problems and short circuit diagnosis
- **[TESTING_GUIDE.md](TESTING_GUIDE.md)** - Complete testing procedures and troubleshooting
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Software architecture and component design

## Quick Start

1. **Hardware:** Build MOSFET buffer circuit (see WIRING_GUIDE.md)
2. **Flash:** `pio run -t upload`
3. **Configure:** Connect to WiFi AP `WaterMeter-ESP32-XXXX`, access WebUI at http://192.168.4.1
4. **Test:** Follow TESTING_GUIDE.md procedures

## Features

- Pulse counting with hardware debounce (500ms)
- Boot protection (3s initialization delay)
- NVS storage (auto-save every 30s)
- Home Assistant auto-discovery (12 entities)
- WebUI configuration
- Telnet console (port 23)
- OTA updates

## Hardware Requirements

- ESP32 dev board
- MOSFET IRLZ24N/34N/44N (logic-level N-channel)
- 2x 10kΩ resistors
- 1x 100nF capacitor (optional but recommended)
- Magnetic pulse sensor output (3.5V HIGH / 0.2V LOW)

## Pinout

- **GPIO34:** Pulse input (via MOSFET buffer)
- **GPIO32:** Status LED
- **GPIO2:** System status LED (DomoticsCore)

## Software Architecture

### Components

- **WaterMeterComponent:** Custom component (pulse counting, storage, event publishing)
- **DomoticsCore System:** Full stack orchestration (WiFi, WebUI, MQTT, HomeAssistant, OTA, NTP)

### Boot Protection

**Problem:** False pulse detection during ESP32 startup.

**Solution:** ISR ignores all pulses for first 3 seconds after boot.

**Implementation:**
```cpp
// In ISR (must use manual millis() check - no C++ objects allowed)
if (!g_initializationComplete) {
    if (currentTime - g_bootTime < BOOT_INIT_DELAY_MS) {
        return;  // Ignore pulse
    }
    g_initializationComplete = true;
}
```

**Why not NonBlockingDelay?** ISR context prohibits C++ object usage (ESP32 IRAM constraints).

### Signal Flow

1. **Water flows** → Magnetic rotor turns
2. **Magnet approaches** → Sensor 3.5V→0.2V → MOSFET ON → GPIO34 LOW (ignored)
3. **Magnet under sensor** → Sensor stays 0.2V → GPIO34 stays LOW
4. **Magnet leaves** → Sensor 0.2V→3.5V → MOSFET OFF → GPIO34 HIGH (**FALLING edge = COUNT**)

### Why FALLING Edge?

MOSFET inverts signal:
- Sensor HIGH (3.5V) → GPIO34 LOW
- Sensor LOW (0.2V) → GPIO34 HIGH

FALLING edge on GPIO34 = sensor rising from LOW to HIGH = liter complete ✓

## Configuration

Edit `include/WaterMeterConfig.h`:

```cpp
#define PULSE_INPUT_PIN 34          // Pulse input via MOSFET
#define STATUS_LED_PIN 32           // Status LED
#define LITERS_PER_PULSE 1.0        // Calibration
#define PULSE_DEBOUNCE_MS 500       // Debounce time
#define BOOT_INIT_DELAY_MS 3000     // Boot protection delay
```

## Home Assistant Integration

### Auto-Discovery Entities (12 total)

**Sensors (7):**
- Total Water Volume (m³)
- Total Liters (L)
- Daily Consumption (m³)
- Daily Liters (L)
- Yearly Consumption (m³)
- Yearly Liters (L)
- Total Pulses

**System Sensors (2):**
- WiFi Signal (dBm)
- Uptime (seconds)

**Buttons (3):**
- Reset Daily Counter
- Reset Yearly Counter
- Restart Device

### MQTT Topics

Published states: `homeassistant/sensor/watermeter-esp32_<entity>/state`

## Console Commands

Via telnet (port 23) or serial:

- `water` - Show current consumption
- `reset_daily` - Reset daily counter to 0
- `reset_yearly` - Reset yearly counter to 0
- `help` - Show all available commands

## Troubleshooting

See [TESTING_GUIDE.md](TESTING_GUIDE.md#troubleshooting) for detailed diagnostics.

**Common issues:**
- **No pulses:** Check MOSFET G-D-S pinout
- **Continuous pulses:** DRAIN/SOURCE swapped
- **Main box affected:** Missing 10kΩ gate pull-down
- **False boot counts:** Increase `BOOT_INIT_DELAY_MS` to 5000

## Code Quality Standards

### Single Responsibility Principle

Each file has one responsibility:
- `WaterMeterComponent.h` - Pulse counting logic
- `WaterMeterWebUI.h` - WebUI provider
- `WaterMeterConfig.h` - Configuration constants
- `main.cpp` - System orchestration

### File Size

Target: 200-800 lines per file ✓

### Testing

All modifications must be tested:
- Unit tests (if applicable)
- Integration tests (compile + flash + verify behavior)

### Non-Blocking Patterns

Use `NonBlockingDelay` for periodic tasks in `loop()`:
```cpp
Utils::NonBlockingDelay timer(5000);

void loop() {
    if (timer.isReady()) {
        // Periodic task
    }
}
```

**Exception:** ISR context requires manual `millis()` checks (C++ object constraint).

## Memory Usage

Current build (DomoticsCore v1.1.4):
- **RAM:** 14.8% (48,408 bytes)
- **Flash:** 73.0% (1,435,553 bytes)

## Version

Current: **1.0.0** (boot protection update)

DomoticsCore: **v1.2.1** (EventBus architecture)

## License

See project root LICENSE file.

## Contributing

1. Follow single responsibility principle
2. Keep files 200-800 lines
3. Test all changes (compile + integration test)
4. Document in English
5. Use NonBlockingDelay (except in ISR)
6. No git commits without approval

---

**For installation instructions:** Start with [WIRING_GUIDE.md](WIRING_GUIDE.md)

**For testing procedures:** See [TESTING_GUIDE.md](TESTING_GUIDE.md)
