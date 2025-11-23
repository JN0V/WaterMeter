/**
 * @file main.cpp
 * @brief ESP32 Water Meter Application
 * @version 0.9.2
 * 
 * Full-featured IoT water meter using DomoticsCore v1.2.1:
 * - WiFi with AP fallback
 * - WebUI on port 80
 * - MQTT publishing (EventBus-based)
 * - Home Assistant integration (auto-discovery)
 * - Telnet console (port 23)
 * - OTA updates
 * - NTP time sync
 * 
 * DomoticsCore v1.2.1 features:
 * - EventBus-only communication (no direct callbacks)
 * - Improved discovery timing (works after WebUI config)
 * - Core::on<>() and Core::emit() helpers
 * 
 * Hardware: ESP32 with magnetic pulse sensor on GPIO34
 */

#include <Arduino.h>
#include <DomoticsCore/System.h>
#include <DomoticsCore/Logger.h>
#include <DomoticsCore/Wifi.h>
#include <DomoticsCore/WebUI.h>
#include <DomoticsCore/MQTT.h>
#include <DomoticsCore/HomeAssistant.h>
#include <DomoticsCore/Timer.h>
#include "WaterMeterComponent.h"
#include "WaterMeterWebUI.h"

using namespace DomoticsCore;
using namespace DomoticsCore::Components;

#define LOG_APP "APP"

// Global instances
System* domotics = nullptr;
WaterMeterComponent* waterMeter = nullptr;
HomeAssistant::HomeAssistantComponent* haPtr = nullptr;
MQTTComponent* mqttPtr = nullptr;

// State tracking for HA
bool initialStatePublished = false;

void setup() {
    Serial.begin(115200);
    delay(100);  // Brief delay for serial
    
    // DomoticsCore full stack configuration
    SystemConfig config = SystemConfig::fullStack();
    config.deviceName = "WaterMeter-ESP32";
    config.wifiSSID = "";         // Empty = AP mode (SSID: WaterMeter-ESP32-XXXX)
    config.wifiPassword = "";     // Empty = open network (no password)
    config.storageNamespace = "watermeter";  // Isolated NVS namespace
    config.ledPin = 2;            // System status LED (built-in)
    
    // MQTT configuration (like FullStack example)
    config.mqttBroker = "";                 // Your MQTT broker IP (empty = no MQTT)
    config.mqttPort = 1883;
    config.mqttUser = "";                  // Empty if no auth
    config.mqttPassword = "";              // Empty if no auth
    config.mqttClientId = config.deviceName;
    
    domotics = new System(config);
    
    // Add WaterMeter component (can be before or after begin() thanks to lazy Core injection)
    waterMeter = new WaterMeterComponent();
    domotics->getCore().addComponent(std::unique_ptr<WaterMeterComponent>(waterMeter));
    
    if (!domotics->begin()) {
        DLOG_E(LOG_APP, "System initialization failed!");
        while(1) {
            domotics->loop();
            yield();  // Non-blocking, feeds watchdog
        }
    }
    
    // Register WaterMeter WebUI provider
    auto* webui = domotics->getCore().getComponent<WebUIComponent>("WebUI");
    if (webui && waterMeter) {
        webui->registerProviderWithComponent(new WaterMeterWebUIProvider(waterMeter), waterMeter);
        DLOG_I(LOG_APP, "✓ WaterMeter WebUI provider registered");
    }
    
    // ========================================================================
    // HOME ASSISTANT INTEGRATION
    // ========================================================================
    // v1.2.1: HomeAssistant uses EventBus for MQTT (no direct dependency)
    // System.h orchestrates: WiFi → MQTT → HA Discovery → NTP
    
    // Get component references
    mqttPtr = domotics->getCore().getComponent<MQTTComponent>("MQTT");
    haPtr = domotics->getCore().getComponent<HomeAssistant::HomeAssistantComponent>("HomeAssistant");
    
    if (haPtr && mqttPtr) {
        DLOG_I(LOG_APP, "Setting up Home Assistant entities...");
        
        // Water meter sensors
        haPtr->addSensor("total_volume", "Total Water Volume", "m³", "water", "mdi:water-outline");
        haPtr->addSensor("total_liters", "Total Liters", "L", "water", "mdi:water-outline");
        haPtr->addSensor("daily_volume", "Daily Consumption", "m³", "water", "mdi:water-outline");
        haPtr->addSensor("daily_liters", "Daily Liters", "L", "water", "mdi:water-outline");
        haPtr->addSensor("yearly_volume", "Yearly Consumption", "m³", "water", "mdi:water-pump");
        haPtr->addSensor("yearly_liters", "Yearly Liters", "L", "water", "mdi:water-pump");
        haPtr->addSensor("pulse_count", "Total Pulses", "", "", "mdi:counter");
        
        // System sensors
        haPtr->addSensor("wifi_signal", "WiFi Signal", "dBm", "signal_strength", "mdi:wifi");
        haPtr->addSensor("uptime", "Uptime", "s", "", "mdi:clock-outline");
        
        // Reset buttons
        haPtr->addButton("reset_daily", "Reset Daily Counter", []() {
            if (waterMeter) {
                waterMeter->resetDaily();
                DLOG_I(LOG_APP, "Daily counter reset from Home Assistant");
            }
        }, "mdi:refresh");
        
        haPtr->addButton("reset_yearly", "Reset Yearly Counter", []() {
            if (waterMeter) {
                waterMeter->resetYearly();
                DLOG_I(LOG_APP, "Yearly counter reset from Home Assistant");
            }
        }, "mdi:calendar-refresh");
        
        haPtr->addButton("restart", "Restart Device", []() {
            DLOG_I(LOG_APP, "Restart requested from Home Assistant");
            delay(1000);
            ESP.restart();
        }, "mdi:restart");
        
        DLOG_I(LOG_APP, "✓ Home Assistant entities created (%d entities)", 
               haPtr->getStatistics().entityCount);
        
        // v1.2.1: Discovery is AUTOMATICALLY published via EventBus orchestration:
        // WiFi Connected → MQTT Connect → HA Discovery → NTP Sync
        // No manual discovery needed - works immediately after WebUI config!
        DLOG_I(LOG_APP, "✓ Discovery will auto-publish when MQTT connects (EventBus orchestration)");
    } else {
        if (!haPtr) {
            DLOG_W(LOG_APP, "⚠️  Home Assistant component not available");
            DLOG_I(LOG_APP, "   Configure MQTT broker in WebUI to enable HA integration");
        }
    }
    
    // ========================================================================
    // EVENTBUS INTEGRATION (v1.2.1)
    // ========================================================================
    // Listen to MQTT events for better integration
    domotics->getCore().on<bool>("mqtt/connected", [](const bool&) {
        DLOG_I(LOG_APP, "🔗 MQTT connected via EventBus - WaterMeter ready for HA discovery");
    });
    
    domotics->getCore().on<bool>("mqtt/disconnected", [](const bool&) {
        DLOG_W(LOG_APP, "🔌 MQTT disconnected via EventBus");
    });
    
    DLOG_I(LOG_APP, "=== WaterMeter v" WATER_METER_VERSION " Ready ===");
    DLOG_I(LOG_APP, "WebUI: http://watermeter-esp32.local or http://192.168.4.1");
    DLOG_I(LOG_APP, "Console: telnet IP_ADDRESS (commands: water, reset_daily, reset_yearly)");
    
    // Register console commands for water meter
    domotics->registerCommand("water", [](const String& args) {
        if (!waterMeter) return String("ERROR: WaterMeter not initialized\n");
        
        WaterMeterData data = waterMeter->getData();
        String output;
        output += "=== Water Meter Status ===\n";
        output += "Total:   " + String(data.totalM3, 3) + " m³ (" + String(data.pulseCount) + " pulses)\n";
        output += "Daily:   " + String(data.dailyM3, 3) + " m³ (" + String(data.dailyLiters) + " L)\n";
        output += "Yearly:  " + String(data.yearlyM3, 3) + " m³ (" + String(data.yearlyLiters) + " L)\n";
        output += "\nCommands: water, reset_daily, reset_yearly\n";
        return output;
    });
    
    domotics->registerCommand("reset_daily", [](const String& args) {
        if (!waterMeter) return String("ERROR: WaterMeter not initialized\n");
        waterMeter->resetDaily();
        return String("Daily counter reset to 0\n");
    });
    
    domotics->registerCommand("reset_yearly", [](const String& args) {
        if (!waterMeter) return String("ERROR: WaterMeter not initialized\n");
        waterMeter->resetYearly();
        return String("Yearly counter reset to 0\n");
    });
}

// Non-blocking timer for MQTT publishing
Utils::NonBlockingDelay mqttPublishTimer(60000);  // 60 seconds (water consumption changes slowly)

void loop() {
    // DomoticsCore System handles everything
    // WaterMeter component loop is called automatically
    domotics->loop();
    
    // ========================================================================
    // PUBLISH INITIAL STATE (once HA is ready)
    // ========================================================================
    if (!initialStatePublished && haPtr && haPtr->isReady() && waterMeter) {
        WaterMeterData data = waterMeter->getData();
        
        // Publish all water meter values (cast double to float for publishState)
        haPtr->publishState("total_volume", (float)data.totalM3);
        haPtr->publishState("total_liters", (float)data.pulseCount);  // 1 pulse = 1 liter
        haPtr->publishState("daily_volume", (float)data.dailyM3);
        haPtr->publishState("daily_liters", (float)data.dailyLiters);
        haPtr->publishState("yearly_volume", (float)data.yearlyM3);
        haPtr->publishState("yearly_liters", (float)data.yearlyLiters);
        haPtr->publishState("pulse_count", (float)data.pulseCount);
        
        initialStatePublished = true;
        DLOG_I(LOG_APP, "✓ Published initial water meter state to Home Assistant");
    }
    
    // ========================================================================
    // MQTT STATE PUBLISHING (to Home Assistant)
    // ========================================================================
    if (mqttPublishTimer.isReady() && haPtr && haPtr->isMQTTConnected() && waterMeter) {
        WaterMeterData data = waterMeter->getData();
        
        // Publish water meter data (cast double to float)
        haPtr->publishState("total_volume", (float)data.totalM3);
        haPtr->publishState("total_liters", (float)data.pulseCount);
        haPtr->publishState("daily_volume", (float)data.dailyM3);
        haPtr->publishState("daily_liters", (float)data.dailyLiters);
        haPtr->publishState("yearly_volume", (float)data.yearlyM3);
        haPtr->publishState("yearly_liters", (float)data.yearlyLiters);
        haPtr->publishState("pulse_count", (float)data.pulseCount);
        
        // System metrics
        haPtr->publishState("uptime", (float)(millis() / 1000));
        
        // WiFi signal if connected
        auto* wifiComp = domotics->getWiFi();
        if (wifiComp && wifiComp->isSTAConnected()) {
            int32_t rssi = wifiComp->getRSSI();
            haPtr->publishState("wifi_signal", (float)rssi);
        }
        
        DLOG_D(LOG_APP, "📡 Published to HA: Total=%.3f m³, Daily=%.3f m³, Yearly=%.3f m³",
               data.totalM3, data.dailyM3, data.yearlyM3);
    }
}
