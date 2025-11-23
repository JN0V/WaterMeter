# WaterMeter Configuration Pattern Migration

**Date:** November 17, 2025  
**Version:** 1.0.0  
**Status:** ✅ Complete

## Summary

Successfully migrated WaterMeterComponent to DomoticsCore standard configuration pattern.

## Changes Applied

### 1. Configuration Structure (`WaterMeterConfig.h`)

Created `WaterMeterConfig` struct following DomoticsCore pattern:

```cpp
struct WaterMeterConfig {
    // Hardware Configuration
    uint8_t pulseInputPin = 34;
    uint8_t statusLedPin = 32;
    
    // Water Meter Settings
    float litersPerPulse = 1.0;
    uint32_t pulseDebounceMs = 500;
    uint32_t bootInitDelayMs = 3000;
    
    // Timing Configuration
    uint32_t saveIntervalMs = 30000;
    uint32_t publishIntervalMs = 5000;
    uint32_t ledFlashMs = 50;
    
    // Feature Flags
    bool enabled = true;
    bool enableLed = true;
};
```

**Benefits:**
- All configurable parameters in one place
- Sensible defaults for all fields
- Type-safe configuration
- Easy to extend

### 2. Component Implementation (`WaterMeterComponent.h`)

#### Constructor
```cpp
explicit WaterMeterComponent(const WaterMeterConfig& cfg = WaterMeterConfig())
    : config(cfg),
      saveTimer(cfg.saveIntervalMs),
      publishTimer(cfg.publishIntervalMs),
      ledTimer(cfg.ledFlashMs)
```

#### Configuration Methods

**getConfig()**: Returns current configuration
```cpp
WaterMeterConfig getConfig() const {
    return config;
}
```

**setConfig()**: Updates configuration intelligently
```cpp
void setConfig(const WaterMeterConfig& cfg) {
    // Detects changes
    // Only restarts if hardware changed
    // Updates timers if intervals changed
}
```

### 3. ISR Configuration

Global variables for ISR (required due to IRAM constraints):
```cpp
volatile uint32_t g_pulseDebounceMs = 500;
volatile uint32_t g_bootInitDelayMs = 3000;
```

Updated during `begin()` from config values.

### 4. Removed Constants

Replaced hardcoded `#define` constants with config struct:
- ❌ `PULSE_INPUT_PIN` → ✅ `config.pulseInputPin`
- ❌ `STATUS_LED_PIN` → ✅ `config.statusLedPin`
- ❌ `LITERS_PER_PULSE` → ✅ `config.litersPerPulse`
- ❌ `PULSE_DEBOUNCE_MS` → ✅ `config.pulseDebounceMs`
- ❌ `BOOT_INIT_DELAY_MS` → ✅ `config.bootInitDelayMs`
- ❌ `SAVE_INTERVAL_MS` → ✅ `config.saveIntervalMs`

## Compilation Results

✅ **SUCCESS**
- RAM: 14.8% (48,424 bytes)
- Flash: 73.0% (1,434,921 bytes)
- +688 bytes vs previous version (acceptable for added flexibility)

## Next Steps

### Recommended: Storage Integration

Add to `main.cpp` after `domotics->begin()`:

```cpp
// Load WaterMeter configuration from Storage
auto* storage = domotics->getCore().getComponent<Components::StorageComponent>("Storage");
auto* waterMeter = domotics->getCore().getComponent<WaterMeterComponent>("WaterMeter");
if (storage && waterMeter) {
    // 1. GET current config (preserves defaults)
    WaterMeterConfig cfg = waterMeter->getConfig();
    
    // 2. OVERRIDE from Storage (uses stored value OR falls back to current)
    cfg.pulseInputPin = storage->getUInt("wm_pulse_pin", cfg.pulseInputPin);
    cfg.statusLedPin = storage->getUInt("wm_led_pin", cfg.statusLedPin);
    cfg.litersPerPulse = storage->getFloat("wm_liters", cfg.litersPerPulse);
    cfg.pulseDebounceMs = storage->getULong("wm_debounce", cfg.pulseDebounceMs);
    cfg.bootInitDelayMs = storage->getULong("wm_boot_delay", cfg.bootInitDelayMs);
    cfg.saveIntervalMs = storage->getULong("wm_save_int", cfg.saveIntervalMs);
    cfg.publishIntervalMs = storage->getULong("wm_pub_int", cfg.publishIntervalMs);
    cfg.enabled = storage->getBool("wm_enabled", cfg.enabled);
    cfg.enableLed = storage->getBool("wm_led_en", cfg.enableLed);
    
    DLOG_I(LOG_APP, "Loading WaterMeter config from Storage");
    
    // 3. SET merged config
    waterMeter->setConfig(cfg);
}
```

### Optional: WebUI Configuration

Add to `WaterMeterWebUIProvider` for runtime config changes:

```cpp
void onConfigChanged(const WaterMeterConfig& cfg) {
    auto* storage = getCore()->getComponent<StorageComponent>("Storage");
    if (storage) {
        storage->putUInt("wm_pulse_pin", cfg.pulseInputPin);
        storage->putFloat("wm_liters", cfg.litersPerPulse);
        // ... save all config fields
        
        DLOG_I(LOG_WATER, "Config saved to Storage");
    }
    
    // Apply immediately
    waterMeter->setConfig(cfg);
}
```

## References

- **DomoticsCore Pattern Guide**: `DomoticsCore/docs/architecture/component-configuration-pattern.md`
- **NTP Implementation Example**: `DomoticsCore-NTP/include/DomoticsCore/NTP.h`
- **Storage Integration Example**: `DomoticsCore-System/include/DomoticsCore/System.h`

## Migration Checklist

- [x] Create `WaterMeterConfig` struct
- [x] Add `getConfig()` method
- [x] Add `setConfig()` method with smart update detection
- [x] Update constructor to accept config
- [x] Replace all #define constants with config fields
- [x] Update ISR to use config values via global variables
- [x] Test compilation
- [ ] Add Storage integration in System (optional)
- [ ] Add WebUI config management (optional)
- [ ] Test with empty Storage (should use defaults)
- [ ] Test with partial Storage (should merge correctly)
- [ ] Update documentation

## Notes

- **Backward Compatibility**: Default config values match original hardcoded constants
- **Runtime State**: Counter data (pulseCount, dailyLiters, yearlyLiters) remains separate from configuration
- **ISR Constraints**: Debounce and boot delay must use global variables due to IRAM requirements
- **Smart Updates**: `setConfig()` only restarts component if hardware pins change
