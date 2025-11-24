#ifndef WATER_METER_CONFIG_H
#define WATER_METER_CONFIG_H

#include <Arduino.h>

// Water Meter Version
#define WATER_METER_VERSION "1.0.0"

/**
 * @brief WaterMeter configuration structure
 * 
 * Follows DomoticsCore standard configuration pattern.
 * All configurable parameters with sensible defaults.
 */
struct WaterMeterConfig {
    // Hardware Configuration
    uint8_t pulseInputPin = 34;        // GPIO pin for pulse detection (input-only, interrupt capable)
    uint8_t statusLedPin = 32;         // External LED for status indication (GPIO32: high-Z when ESP32 off)
    
    // Water Meter Settings
    float litersPerPulse = 1.0;        // Volume per pulse in liters
    uint32_t pulseDebounceMs = 500;    // Debounce time in milliseconds (for magnetic sensor)
    uint32_t pulseHighStableMs = 150;  // Minimum stable HIGH time required before accepting next pulse
    uint32_t bootInitDelayMs = 3000;   // Boot initialization delay (no pulse counting for 3 seconds)
    
    // Timing Configuration
    uint32_t saveIntervalMs = 30000;   // Save data every 30 seconds
    uint32_t publishIntervalMs = 5000; // Publish data every 5 seconds
    uint32_t ledFlashMs = 50;          // LED flash duration
    
    // Feature Flags
    bool enabled = true;               // Enable/disable component
    bool enableLed = true;             // Enable/disable LED feedback
};

// Water Meter specific log tags
#define LOG_WATER       "WATER"       // Water meter component logs
#define LOG_SENSOR      "SENSOR"      // Sensor/pulse detection logs

#endif // WATER_METER_CONFIG_H
