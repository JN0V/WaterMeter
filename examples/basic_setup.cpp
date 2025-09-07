#include <Arduino.h>
#include <DomoticsCore/DomoticsCore.h>
#include "WaterMeterConfig.h"

/*
 * Basic Water Meter Setup Example
 * 
 * This example demonstrates the minimal setup required for water meter monitoring
 * with 24V pulse input protection and basic MQTT publishing.
 * 
 * Configuration:
 * 1. Upload this code to your ESP32
 * 2. Connect to the "WaterMeter-XXXXXX" Access Point created by the device
 * 3. Open browser to 192.168.4.1 to configure WiFi and MQTT settings
 * 4. Device will automatically connect and start publishing data
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - 24V water meter with pulse output (1 pulse = 1 liter)
 * - Voltage divider circuit (22kΩ + 3.3kΩ + 3.6V Zener)
 * - External LED on GPIO25 with 220Ω resistor
 * - Breadboard and jumper wires
 * 
 * Wiring Diagram:
 * 
 * Water Meter (24V)     Protection Circuit        ESP32
 * ┌─────────────┐      ┌─────────────────┐      ┌──────────┐
 * │    Pulse    │──────│ 24V ──[22kΩ]──┬─│──────│ GPIO2    │
 * │   Output    │      │              │ │      │          │
 * │             │      │         [3.3kΩ] │      │          │
 * │     GND     │──────│              │ │      │          │
 * └─────────────┘      │             GND │      │   GND    │
 *                      │                 │      │          │
 *                      │        [Zener]  │      │          │
 *                      │         3.6V    │      │          │
 *                      │           │     │      │          │
 *                      │          GND    │      │          │
 *                      └─────────────────┘      └──────────┘
 */

// Global variables
volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
DomoticsCore domotics;

// Interrupt service routine
void IRAM_ATTR pulseISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastPulseTime > PULSE_DEBOUNCE_MS) {
        pulseCount++;
        lastPulseTime = currentTime;
        Serial.printf("Pulse detected! Total: %lu\n", pulseCount);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Water Meter Basic Setup Example");
    
    // Initialize GPIO
    pinMode(PULSE_INPUT_PIN, INPUT_PULLDOWN);
    pinMode(STATUS_LED_PIN, OUTPUT);
    
    // Attach interrupt
    attachInterrupt(digitalPinToInterrupt(PULSE_INPUT_PIN), pulseISR, RISING);
    
    // Initialize DomoticsCore
    domotics.begin();
    domotics.setDeviceType("WaterMeter");
    
    // Enable services - DomoticsCore will create an Access Point for configuration
    // Connect to "WaterMeter-XXXXXX" AP and go to 192.168.4.1 to configure
    domotics.enableWiFi();
    domotics.enableMQTT();
    
    Serial.println("Setup complete. Monitoring water pulses...");
}

void loop() {
    static unsigned long lastPublish = 0;
    
    // Handle DomoticsCore tasks
    domotics.handle();
    
    // Publish data every minute
    if (millis() - lastPublish > 60000) {
        if (domotics.isMQTTConnected()) {
            unsigned long totalLiters = pulseCount * LITERS_PER_PULSE;
            
            String payload = "{";
            payload += "\"total_liters\":" + String(totalLiters) + ",";
            payload += "\"pulse_count\":" + String(pulseCount) + ",";
            payload += "\"uptime\":" + String(millis());
            payload += "}";
            
            domotics.publishMQTT("watermeter/data", payload);
            domotics.publishMQTT("watermeter/total", String(totalLiters));
            
            Serial.printf("Published: Total %lu L, Pulses: %lu\n", totalLiters, pulseCount);
        }
        lastPublish = millis();
    }
    
    // Blink LED to show activity
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastBlink = millis();
    }
    
    delay(100);
}
