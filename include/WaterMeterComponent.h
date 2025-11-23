#pragma once

/**
 * @file WaterMeterComponent.h
 * @brief ESP32 Water Meter Component with pulse counting
 * 
 * Features:
 * - Magnetic sensor pulse detection via NPN transistor (inverted signal)
 * - FALLING edge detection = magnet LEAVING sensor (1 liter complete)
 * - Boot initialization delay (no counting for 3 seconds after power-on)
 * - Hardware debounce + software debounce (configurable)
 * - Daily/Yearly consumption tracking
 * - Auto-save to NVS storage every 30s
 * - Event bus data publishing every 5s
 * - LED visual feedback (non-blocking)
 * - Console commands for status and reset
 * 
 * Hardware:
 * - GPIO34: Pulse input via NPN transistor buffer
 *   - Sensor HIGH (3.5V, no magnet) → Transistor ON → GPIO34 LOW
 *   - Sensor LOW (0.2V, magnet) → Transistor OFF → GPIO34 HIGH (pull-up)
 * - GPIO32: Status LED (pulse indicator)
 * 
 * Signal Flow:
 * - Magnet approaches → Sensor 3.5V→0.2V → ESP32 LOW→HIGH (ignored)
 * - Magnet leaves → Sensor 0.2V→3.5V → ESP32 HIGH→LOW = **COUNT** ✓
 * 
 * Note: ISR and volatile variables must be GLOBAL to avoid
 * ESP32 IRAM linker issues with C++ class static members.
 */

#include <DomoticsCore/IComponent.h>
#include <DomoticsCore/Logger.h>
#include <DomoticsCore/Timer.h>
#include <DomoticsCore/Storage.h>
#include <DomoticsCore/Core.h>
#include <time.h>
#include "WaterMeterConfig.h"

using namespace DomoticsCore;
using namespace DomoticsCore::Components;

#define LOG_WATER "WATER"
#define LOG_SENSOR "SENSOR"

// Water meter data for event bus
struct WaterMeterData {
    uint64_t pulseCount;
    uint64_t dailyLiters;
    uint64_t yearlyLiters;
    double totalM3;
    double dailyM3;
    double yearlyM3;
};

// ISR globals - must be outside class to avoid IRAM issues
namespace {
    volatile uint64_t g_pulseCount = 0;
    volatile unsigned long g_lastPulseTime = 0;
    volatile bool g_newPulseDetected = false;
    volatile bool g_pulseIgnored = false;
    volatile unsigned long g_lastIgnoredTimeDiff = 0;
    volatile unsigned long g_bootTime = 0;           // Boot timestamp for initialization delay
    volatile bool g_initializationComplete = false;  // ISR enabled after delay
    volatile bool g_initJustCompleted = false;       // Flag to log init completion (non-ISR)
    
    // Config values used by ISR (set by component during begin())
    volatile uint8_t g_pulsePin = 34;                // Pin number for digitalRead in ISR
    volatile uint32_t g_pulseDebounceMs = 500;       // Debounce time
    volatile uint32_t g_pulseHighStableMs = 150;     // Stable HIGH time required
    volatile uint32_t g_bootInitDelayMs = 3000;      // Boot init delay
    volatile unsigned long g_lastRisingTime = 0;     // Last time signal went HIGH
}

// ISR - global function that works
void IRAM_ATTR waterMeterPulseISR() {
    unsigned long currentTime = millis();
    int pinState = digitalRead(g_pulsePin);
    
    // Ignore pulses during initialization period (prevents boot false positives)
    if (!g_initializationComplete) {
        if (currentTime - g_bootTime < g_bootInitDelayMs) {
            return;  // Silent ignore during boot
        }
        g_initializationComplete = true;
        g_initJustCompleted = true;  // Flag for logging in loop()
        // Initialize lastRisingTime to now to avoid immediate false trigger if we start LOW
        g_lastRisingTime = currentTime;
    }
    
    if (pinState == LOW) { // FALLING EDGE (Potential Pulse)
        unsigned long timeDiff = currentTime - g_lastPulseTime;
        unsigned long stableHighDiff = currentTime - g_lastRisingTime;
        
        // Valid pulse requires:
        // 1. Enough time since last pulse (Debounce)
        // 2. Signal was HIGH for enough time before this FALLING edge (Stability)
        if (timeDiff > g_pulseDebounceMs && stableHighDiff > g_pulseHighStableMs) {
            g_pulseCount++;
            g_lastPulseTime = currentTime;
            g_newPulseDetected = true;
        } else {
            g_pulseIgnored = true;
            g_lastIgnoredTimeDiff = timeDiff;
            // Note: we could also log why it was ignored (debounce vs stability) but keep it simple
        }
    } else { // RISING EDGE (Magnet leaving)
        g_lastRisingTime = currentTime;
    }
}

class WaterMeterComponent : public IComponent {
private:
    WaterMeterConfig config;
    
    // Runtime state data (not configuration)
    uint64_t dailyLiters = 0;
    uint64_t yearlyLiters = 0;
    int lastDay = -1;
    int lastYear = -1;
    
    // Non-blocking timers (initialized in constructor)
    Utils::NonBlockingDelay saveTimer;
    Utils::NonBlockingDelay publishTimer;
    Utils::NonBlockingDelay ledTimer;

public:
    /**
     * @brief Construct WaterMeter component with configuration
     * @param cfg WaterMeter configuration (uses defaults if not provided)
     */
    explicit WaterMeterComponent(const WaterMeterConfig& cfg = WaterMeterConfig())
        : config(cfg),
          saveTimer(cfg.saveIntervalMs),
          publishTimer(cfg.publishIntervalMs),
          ledTimer(cfg.ledFlashMs) {
        metadata.name = "WaterMeter";
        metadata.version = WATER_METER_VERSION;
        metadata.author = "JNOV";
        metadata.description = "Water meter pulse counter with DomoticsCore integration";
    }

    std::vector<Dependency> getDependencies() const override {
        return {
            {"Storage", false},  // Optional - persistent data storage
            {"NTP", false}       // Optional - for time-based resets
        };
    }

    ComponentStatus begin() override {
        if (!config.enabled) {
            DLOG_I(LOG_WATER, "WaterMeter disabled in configuration");
            setActive(false);
            return ComponentStatus::Success;
        }

        DLOG_I(LOG_WATER, "Initializing water meter (pin=%d, led=%d, L/pulse=%.1f)...",
               config.pulseInputPin, config.statusLedPin, config.litersPerPulse);

        // Update global ISR config values
        g_pulsePin = config.pulseInputPin;
        g_pulseDebounceMs = config.pulseDebounceMs;
        g_pulseHighStableMs = config.pulseHighStableMs;
        g_bootInitDelayMs = config.bootInitDelayMs;

        // GPIO setup
        pinMode(config.pulseInputPin, INPUT);
        pinMode(config.statusLedPin, OUTPUT);
        digitalWrite(config.statusLedPin, LOW);

        // Record boot time for initialization delay
        g_bootTime = millis();
        g_initializationComplete = false;
        
        // Read initial GPIO state (for diagnostics)
        int initialState = digitalRead(config.pulseInputPin);
        DLOG_I(LOG_WATER, "Initial GPIO state: %s (magnet %s sensor)",
               initialState ? "HIGH" : "LOW",
               initialState ? "NOT under" : "UNDER");
        
        // Let GPIO stabilize
        delay(100);
        
        // Attach interrupt (CHANGE to detect both edges for stability check)
        attachInterrupt(digitalPinToInterrupt(config.pulseInputPin), waterMeterPulseISR, CHANGE);
        DLOG_I(LOG_WATER, "Interrupt attached to GPIO %d (CHANGE mode for stability check)", config.pulseInputPin);
        DLOG_W(LOG_WATER, "⏳ Pulse detection disabled for %lu ms (boot protection)", config.bootInitDelayMs);
        
        setActive(true);
        
        // Load from storage if available
        loadFromStorage();
        
        DLOG_I(LOG_WATER, "Water meter ready: %llu pulses (%.3f m³)",
               g_pulseCount, g_pulseCount * config.litersPerPulse / 1000.0);
        return ComponentStatus::Success;
    }

    void loop() override {
        // Log initialization completion (outside ISR)
        if (g_initJustCompleted) {
            DLOG_I(LOG_SENSOR, "✓ Pulse detection enabled after %lu ms (boot protection complete)", 
                   millis() - g_bootTime);
            g_initJustCompleted = false;
        }
        
        // Handle new pulse from ISR
        if (g_newPulseDetected) {
            dailyLiters += static_cast<uint64_t>(config.litersPerPulse);
            yearlyLiters += static_cast<uint64_t>(config.litersPerPulse);
            
            DLOG_I(LOG_SENSOR, "PULSE: count=%llu, daily=%lluL, yearly=%lluL", 
                   g_pulseCount, dailyLiters, yearlyLiters);
            
            // LED feedback - non-blocking
            if (config.enableLed) {
                digitalWrite(config.statusLedPin, HIGH);
                ledTimer.reset();
            }
            
            g_newPulseDetected = false;
        }
        
        // Turn off LED after timer
        if (config.enableLed && digitalRead(config.statusLedPin) == HIGH && ledTimer.isReady()) {
            digitalWrite(config.statusLedPin, LOW);
        }
        
        // Log ignored pulses (debounce)
        if (g_pulseIgnored) {
            DLOG_W(LOG_SENSOR, "Pulse ignored (debounce): %lu ms", g_lastIgnoredTimeDiff);
            g_pulseIgnored = false;
        }
        
        // Check for daily/yearly reset (requires NTP)
        checkTimeBasedResets();
        
        // Auto-save with non-blocking timer
        if (saveTimer.isReady()) {
            saveToStorage();
        }
        
        // Publish data with non-blocking timer
        if (publishTimer.isReady()) {
            publishData();
        }
    }

    ComponentStatus shutdown() override {
        if (config.enabled) {
            detachInterrupt(digitalPinToInterrupt(config.pulseInputPin));
        }
        saveToStorage();
        setActive(false);
        DLOG_I(LOG_WATER, "Water meter shutdown");
        return ComponentStatus::Success;
    }

    WaterMeterData getData() const {
        WaterMeterData data;
        data.pulseCount = g_pulseCount;
        data.dailyLiters = dailyLiters;
        data.yearlyLiters = yearlyLiters;
        data.totalM3 = (g_pulseCount * config.litersPerPulse) / 1000.0;
        data.dailyM3 = dailyLiters / 1000.0;
        data.yearlyM3 = yearlyLiters / 1000.0;
        return data;
    }

    /**
     * @brief Get current component configuration
     * @return Current WaterMeterConfig
     */
    WaterMeterConfig getConfig() const {
        return config;
    }

    /**
     * @brief Update configuration after component creation
     * @param cfg New configuration
     * 
     * Detects changes and applies them intelligently.
     * Only restarts if hardware configuration changed.
     */
    void setConfig(const WaterMeterConfig& cfg) {
        // Detect what changed
        bool hardwareChanged = (cfg.pulseInputPin != config.pulseInputPin) ||
                               (cfg.statusLedPin != config.statusLedPin);
        bool enabledChanged = (cfg.enabled != config.enabled);
        bool timersChanged = (cfg.saveIntervalMs != config.saveIntervalMs) ||
                            (cfg.publishIntervalMs != config.publishIntervalMs) ||
                            (cfg.ledFlashMs != config.ledFlashMs);
        
        DLOG_I(LOG_WATER, "Updating config: enabled=%d, pin=%d, led=%d, L/pulse=%.1f, highStable=%dms",
               cfg.enabled, cfg.pulseInputPin, cfg.statusLedPin, cfg.litersPerPulse, cfg.pulseHighStableMs);
        
        // Apply new config
        config = cfg;
        
        // Update ISR globals
        g_pulseDebounceMs = config.pulseDebounceMs;
        g_pulseHighStableMs = config.pulseHighStableMs;
        
        // Update timers if intervals changed
        if (timersChanged) {
            saveTimer = Utils::NonBlockingDelay(config.saveIntervalMs);
            publishTimer = Utils::NonBlockingDelay(config.publishIntervalMs);
            ledTimer = Utils::NonBlockingDelay(config.ledFlashMs);
            DLOG_I(LOG_WATER, "Timers updated: save=%lums, publish=%lums",
                   config.saveIntervalMs, config.publishIntervalMs);
        }
        
        // Restart if hardware config changed or enabled state changed
        if (hardwareChanged || enabledChanged) {
            DLOG_W(LOG_WATER, "Hardware config changed - restart required");
            shutdown();
            begin();
        }
    }

    void resetDaily() {
        dailyLiters = 0;
        saveToStorage();
        DLOG_I(LOG_WATER, "Daily counter reset");
    }

    void resetYearly() {
        yearlyLiters = 0;
        saveToStorage();
        DLOG_I(LOG_WATER, "Yearly counter reset");
    }

    void overridePulseCount(uint64_t newCount) {
        g_pulseCount = newCount;
        saveToStorage();
        DLOG_I(LOG_WATER, "Pulse count overridden to %llu (%.3f m³)", 
               g_pulseCount, g_pulseCount * config.litersPerPulse / 1000.0);
    }

    void overrideDailyLiters(uint64_t newValue) {
        dailyLiters = newValue;
        saveToStorage();
        DLOG_I(LOG_WATER, "Daily liters overridden to %llu L (%.3f m³)", 
               dailyLiters, dailyLiters / 1000.0);
    }

    void overrideYearlyLiters(uint64_t newValue) {
        yearlyLiters = newValue;
        saveToStorage();
        DLOG_I(LOG_WATER, "Yearly liters overridden to %llu L (%.3f m³)", 
               yearlyLiters, yearlyLiters / 1000.0);
    }

private:
    void loadFromStorage() {
        auto* storage = getCore()->getComponent<Components::StorageComponent>("Storage");
        if (!storage) {
            DLOG_W(LOG_WATER, "Storage not available, using defaults");
            return;
        }

        // Load uint64_t using native v1.0.1+ support
        g_pulseCount = storage->getULong64("pulse_count", 0);
        dailyLiters = storage->getULong64("daily_liters", 0);
        yearlyLiters = storage->getULong64("yearly_liters", 0);
        
        DLOG_I(LOG_WATER, "Loaded from storage: %llu pulses, %lluL daily, %lluL yearly",
               g_pulseCount, dailyLiters, yearlyLiters);
    }

    void saveToStorage() {
        auto* storage = getCore()->getComponent<Components::StorageComponent>("Storage");
        if (!storage) {
            DLOG_W(LOG_WATER, "Storage not available, skipping save");
            return;
        }

        // Save uint64_t using native v1.0.1+ support
        storage->putULong64("pulse_count", g_pulseCount);
        storage->putULong64("daily_liters", dailyLiters);
        storage->putULong64("yearly_liters", yearlyLiters);
        
        DLOG_D(LOG_WATER, "Saved: %llu pulses, %lluL daily, %lluL yearly",
               g_pulseCount, dailyLiters, yearlyLiters);
    }

    void checkTimeBasedResets() {
        auto* ntp = getCore()->getComponent("NTP");
        if (!ntp || !ntp->isActive()) {
            return;  // NTP not ready
        }

        time_t now = time(nullptr);
        struct tm timeinfo;
        if (!localtime_r(&now, &timeinfo)) {
            return;  // Time not valid
        }

        int currentDay = timeinfo.tm_yday;   // Day of year (0-365)
        int currentYear = timeinfo.tm_year;  // Years since 1900

        // Daily reset at midnight (day changed)
        if (lastDay != -1 && currentDay != lastDay) {
            DLOG_I(LOG_WATER, "Daily reset triggered (day %d -> %d)", lastDay, currentDay);
            resetDaily();
        }
        lastDay = currentDay;

        // Yearly reset on January 1st (year changed)
        if (lastYear != -1 && currentYear != lastYear) {
            DLOG_I(LOG_WATER, "Yearly reset triggered (year %d -> %d)", lastYear + 1900, currentYear + 1900);
            resetYearly();
        }
        lastYear = currentYear;
    }

    void publishData() {
        WaterMeterData data = getData();
        emit("watermeter.data", data, false);
        
        DLOG_D(LOG_WATER, "Total: %.3f m³, Daily: %llu L, Yearly: %.3f m³", 
               data.totalM3, data.dailyLiters, data.yearlyM3);
    }
};
