#include <Arduino.h>
#include <DomoticsCore/System.h>
#include "../include/WaterMeterConfig.h"
#include "../include/WaterMeterComponent.h"

/*
 * Basic Water Meter Setup Example - DomoticsCore v1.0
 * 
 * This example demonstrates a minimal setup using DomoticsCore v1.0's
 * System architecture with the custom WaterMeterComponent.
 * 
 * Features:
 * - Automatic Access Point mode on first boot
 * - LED status indicators (GPIO 2)
 * - WebUI configuration
 * - MQTT publishing (optional)
 * - Persistent storage
 * 
 * Quick Start:
 * 1. Upload this code to your ESP32
 * 2. Watch LED status:
 *    - Fast blink (200ms) = Booting
 *    - Slow blink (1000ms) = WiFi connecting
 *    - Breathing (3000ms) = Ready!
 * 3. Connect to AP "WaterMeter-ESP32-XXXXXX"
 * 4. Open browser to 192.168.4.1
 * 5. Configure WiFi and MQTT (optional)
 * 
 * Hardware:
 * - ESP32 development board
 * - Water meter with magnetic pulse sensor (1 pulse = 1 liter)
 * - GPIO34 for pulse input
 * - GPIO25 for external status LED
 * - GPIO2 for system LED (built-in)
 */

using namespace DomoticsCore;

// Global system instance
System* system = nullptr;
WaterMeterComponent* waterMeter = nullptr;

void setup() {
    Serial.begin(115200);
    
    DLOG_I(LOG_APP, "Water Meter Basic Example - DomoticsCore v1.0");
    DLOG_I(LOG_APP, "Version: %s", WATER_METER_VERSION);
    
    // Configure DomoticsCore System
    SystemConfig config = SystemConfig::fullStack();
    config.deviceName = "WaterMeter-ESP32";
    config.firmwareVersion = WATER_METER_VERSION;
    config.manufacturer = "JNOV";
    
    // Empty WiFi = Access Point mode
    config.wifiSSID = "";
    config.wifiPassword = "";
    
    // LED status on GPIO 2
    config.ledPin = 2;
    
    // MQTT disabled by default (configure via WebUI)
    config.mqttBroker = "";
    config.mqttPort = 1883;
    
    // Create system
    system = new System(config);
    
    // Register telnet command to view water data
    system->registerCommand("water", [](const String& args) {
        if (!waterMeter) return String("WaterMeter not initialized\n");
        
        WaterMeterData data = waterMeter->getData();
        String output;
        output += "Water Meter Status:\n";
        output += "  Total: " + String(data.totalM3, 3) + " m³\n";
        output += "  Daily: " + String(data.dailyLiters) + " L\n";
        output += "  Pulses: " + String((unsigned long)data.pulseCount) + "\n";
        return output;
    });
    
    // Initialize system
    if (!system->begin()) {
        DLOG_E(LOG_APP, "System initialization failed!");
        while (1) {
            system->loop(); // Keep LED error indicator
            delay(100);
        }
    }
    
    // Add WaterMeter component
    waterMeter = new WaterMeterComponent();
    system->getCore().addComponent(std::unique_ptr<WaterMeterComponent>(waterMeter));
    
    DLOG_I(LOG_APP, "Setup complete!");
    DLOG_I(LOG_APP, "Connect to AP or access WebUI at http://192.168.4.1");
}

void loop() {
    // DomoticsCore System handles everything automatically:
    // - WiFi connection and AP mode
    // - LED status indicators
    // - WebUI
    // - MQTT publishing (via event bus)
    // - WaterMeter component loop
    // - Remote console
    // - OTA updates
    system->loop();
}
