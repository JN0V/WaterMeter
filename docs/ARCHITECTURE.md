# WaterMeter Architecture v0.5.0

## Overview

ESP32-based IoT water meter using **DomoticsCore v1.0** framework with clean component architecture and proper ISR handling.

## Project Structure

```
WaterMeter/
├── src/
│   └── main.cpp              # Application entry point (92 lines)
├── include/
│   ├── WaterMeterComponent.h # Core water meter logic (228 lines)
│   └── WaterMeterConfig.h    # Hardware configuration
├── platformio.ini            # Build configuration
└── README.md                 # User documentation
```

## Component Architecture

### main.cpp - Application Bootstrap
**Purpose**: Minimal setup, delegates to DomoticsCore System

**Responsibilities**:
- Initialize DomoticsCore with `System::fullStack()`
- Register `WaterMeterComponent`
- Register console commands (`water`, `reset_daily`, `reset_yearly`)

**Size**: 92 lines (70% documentation)

### WaterMeterComponent - Business Logic
**Purpose**: Encapsulates all water meter functionality

**Responsibilities**:
- GPIO initialization and interrupt management
- Pulse counting with hardware debouncing
- Daily/yearly consumption tracking
- Non-blocking LED feedback (3 timers: LED 50ms, save 30s, publish 5s)
- Storage persistence (NVS via blob storage for uint64_t)
- Event bus data publishing
- Time-based auto-resets (NTP-based: midnight daily, Jan 1st yearly)

**Core Access Pattern** (v1.1.0+):
```cpp
// v1.1.0+ Optional Dependencies + Lifecycle Callback
class WaterMeterComponent : public IComponent {
    std::vector<Dependency> getDependencies() const override {
        return {
            {"Storage", false},  // Optional - explicit intent!
            {"NTP", false}       // Optional
        };
    }
    
    ComponentStatus begin() override {
        // Internal initialization only (GPIO, state)
        pinMode(PULSE_INPUT_PIN, INPUT);
        attachInterrupt(pin, waterMeterPulseISR, FALLING);
        return ComponentStatus::Success;
    }
    
    void afterAllComponentsReady() override {
        // All components (including built-ins) guaranteed ready here
        loadFromStorage();  // Safe access to Storage
    }
};
```

✨ **v1.1.0+ Enhancements:**
- **Optional Dependencies**: Declare Storage/NTP with `required=false` flag
- **Lifecycle Callback**: `afterAllComponentsReady()` called after all components init
- **Clean Separation**: `begin()` = GPIO, `afterAllComponentsReady()` = dependencies
- **Lazy Core Injection** (v1.0.2): Components can be added before or after `begin()`

**Size**: 265 lines with full documentation (25 lines saved vs v1.0.0!)

## ISR Implementation - Critical Pattern

### The Problem
ESP32 linker **cannot** place literals in IRAM when ISR is a C++ class static member:
```cpp
class Component {
    static volatile uint64_t count;  // ❌ Causes linker error
    static void IRAM_ATTR isr() {
        count++;  // "dangerous relocation: l32r"
    }
};
```

### The Solution
ISR and volatile variables **must be global** (outside class):

```cpp
// CORRECT: Global ISR pattern
namespace {
    volatile uint64_t g_pulseCount = 0;
    volatile bool g_newPulse = false;
}

void IRAM_ATTR waterMeterPulseISR() {
    unsigned long now = millis();
    if (now - g_lastPulseTime > DEBOUNCE_MS) {
        g_pulseCount++;
        g_newPulse = true;
    }
}

class WaterMeterComponent {
    void loop() {
        if (g_newPulse) { /* handle */ }
    }
};
```

**Why?**
- ESP32 toolchain limitation with C++ class static members in IRAM
- Global functions work because linker can place them correctly
- Anonymous namespace keeps variables file-scoped

**Failed Attempts**:
- `DRAM_ATTR` / `IRAM_ATTR` attributes ❌
- `__attribute__((section(".dram0.data")))` ❌
- Using uint32_t instead of uint64_t ❌

## Data Flow

```
Hardware Sensor (3.5V pulse)
    ↓
GPIO34 Interrupt (FALLING edge)
    ↓
waterMeterPulseISR() → g_pulseCount++ [IRAM]
    ↓
WaterMeterComponent::loop() [main]
    ↓
├─→ Update daily/yearly counters
├─→ LED feedback (non-blocking timer)
├─→ Auto-save to NVS (every 30s)
├─→ Publish to event bus (every 5s)
└─→ Check time-based resets (NTP)
    ↓
Event Bus → MQTT → Home Assistant
```

## Non-Blocking Design

### Problem: Blocking delays in loop()
```cpp
// ❌ BAD: Blocks entire system
digitalWrite(LED, HIGH);
delay(50);
digitalWrite(LED, LOW);
```

### Solution: NonBlockingDelay timers
```cpp
// ✅ GOOD: Non-blocking with timer
Utils::NonBlockingDelay ledTimer{50};

if (newPulse) {
    digitalWrite(LED, HIGH);
    ledTimer.reset();
}

if (digitalRead(LED) == HIGH && ledTimer.isReady()) {
    digitalWrite(LED, LOW);
}
```

**Benefits**:
- System remains responsive
- Other components can run
- Network/MQTT/WebUI not blocked

## DomoticsCore v1.0 API

### Component Lifecycle
```cpp
class WaterMeterComponent : public IComponent {
    ComponentStatus begin() override {
        // Initialize hardware
        // Load from storage
        // Attach interrupts
        return ComponentStatus::Success;
    }
    
    void loop() override {
        // Non-blocking processing
        // Timer-based operations
    }
    
    ComponentStatus shutdown() override {
        // Detach interrupts
        // Save to storage
        return ComponentStatus::Success;
    }
};
```

### Event Bus Usage
```cpp
// Emit data to event bus
WaterMeterData data = getData();
emit("watermeter.data", data, false);

// Subscribe in other components
on<WaterMeterData>("watermeter.data", [](const WaterMeterData& data) {
    // Handle data
});
```

### Storage (✅ Implemented - v1.0.1+)
```cpp
// Load from NVS with native uint64_t support (v1.0.1+)
void loadFromStorage() {
    auto* storage = getCore()->getComponent<StorageComponent>("Storage");
    g_pulseCount = storage->getULong64("pulse_count", 0);
    dailyLiters = storage->getULong64("daily_liters", 0);
}

// Save to NVS with native uint64_t support
void saveToStorage() {
    auto* storage = getCore()->getComponent<StorageComponent>("Storage");
    storage->putULong64("pulse_count", g_pulseCount);
    storage->putULong64("daily_liters", dailyLiters);
}
```

**Note**: DomoticsCore v1.0.1+ has native `putULong64/getULong64` support - much cleaner than the blob workaround!

## Memory Usage

**Compilation Results** (v0.5.0):
- **RAM**: 14.8% (48,376 / 327,680 bytes)
- **Flash**: 70.6% (1,388,785 / 1,966,080 bytes)

**Breakdown**:
- DomoticsCore framework: ~550KB
- WaterMeterComponent: ~10KB
- ESP32 Arduino core: ~400KB
- WebUI assets: ~150KB

## Configuration

### Hardware (WaterMeterConfig.h)
```cpp
#define PULSE_INPUT_PIN      34    // Interrupt capable
#define STATUS_LED_PIN       25    // Visual feedback
#define LITERS_PER_PULSE     1     // Sensor calibration
#define PULSE_DEBOUNCE_MS    500   // Software debounce
```

### Software (main.cpp)
```cpp
SystemConfig config = SystemConfig::fullStack();
config.deviceName = "WaterMeter-ESP32";
config.wifiSSID = "";         // AP mode
config.wifiPassword = "";
config.ledPin = 2;            // System LED
```

## Console Commands

Via Telnet (port 23):
```bash
> water              # Show current status
> reset_daily        # Reset daily counter
> reset_yearly       # Reset yearly counter
> help               # List all commands (DomoticsCore)
```

## Build & Deploy

```bash
# Compile
pio run

# Upload (USB)
pio run -t upload

# Monitor serial
pio device monitor

# Clean build
pio run -t clean
```

## Implemented Features ✅

1. **Storage Integration** - ✅ NVS persistence with native uint64_t (v1.0.1+)
2. **NTP Integration** - ✅ Auto-resets at midnight (daily) and Jan 1st (yearly)
3. **Non-blocking Timers** - ✅ LED, save, publish timers
4. **Event Bus** - ✅ Data publishing for MQTT/HA
5. **Console Commands** - ✅ water, reset_daily, reset_yearly
6. **Core Access** - ✅ Automatic injection via getCore() (v1.0.1+)

## Future Enhancements

1. **WebUI Schema** - Configuration interface for calibration (liters_per_pulse, debounce)
2. **MQTT Topics** - Custom topic configuration via WebUI
3. **HA Discovery** - Enhanced sensor metadata (device_class, state_class)
4. **Historical Data** - Daily/monthly consumption charts
5. **Flow Rate Detection** - Real-time flow calculation

## Key Learnings

1. **ISR must be global** - ESP32 IRAM limitation with C++ classes
2. **Use NonBlockingDelay** - Never block in component loop()
3. **DomoticsCore handles infrastructure** - Focus on business logic
4. **Event bus for decoupling** - Components don't depend on each other
5. **Proper documentation** - Critical for maintainability

## References

- [DomoticsCore v1.0](https://github.com/JN0V/DomoticsCore)
- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [PlatformIO Documentation](https://docs.platformio.org/)
