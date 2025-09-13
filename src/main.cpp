#include <DomoticsCore/DomoticsCore.h>
#include <DomoticsCore/Logger.h>
#include <ArduinoJson.h>
#include "WaterMeterConfig.h"
#include "web_content.h"

// Application logging tags
#define LOG_WATER_METER "WATER"
#define LOG_SENSOR      "SENSOR"

CoreConfig cfg;
DomoticsCore* gCore = nullptr;

// Water meter variables - using uint64_t for large counts
volatile uint64_t pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
uint64_t dailyLiters = 0;
uint64_t yearlyLiters = 0;
static int lastDay = -1;
static int lastYear = -1;

// ISR flags for logging (avoid logging in interrupt context)
volatile bool newPulseDetected = false;
volatile bool pulseIgnored = false;
volatile unsigned long lastIgnoredTimeDiff = 0;

// DomoticsCore storage
void loadFromStorage() {
    pulseCount = gCore->storage().getULong64("pulse_count", 0);
    dailyLiters = gCore->storage().getULong64("daily_liters", 0);
    yearlyLiters = gCore->storage().getULong64("yearly_liters", 0);
    DLOG_I(LOG_WATER_METER, "Loaded from storage: %llu pulses, %llu L daily, %llu L yearly", pulseCount, dailyLiters, yearlyLiters);
}

void saveToStorage() {
    gCore->storage().putULong64("pulse_count", pulseCount);
    gCore->storage().putULong64("daily_liters", dailyLiters);
    gCore->storage().putULong64("yearly_liters", yearlyLiters);
}

// Interrupt for pulse detection - magnetic sensor with FALLING edge (magnet passes)
void IRAM_ATTR pulseISR() {
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastPulseTime;
    
    // Simple debounce for magnetic sensor
    if (timeDiff > PULSE_DEBOUNCE_MS) {
        pulseCount++;
        lastPulseTime = currentTime;
        newPulseDetected = true;
    } else {
        pulseIgnored = true;
        lastIgnoredTimeDiff = timeDiff;
    }
}

void setup() {
    // Configure DomoticsCore
    cfg.deviceName = "WaterMeter-ESP32";
    cfg.firmwareVersion = WATER_METER_VERSION;
    cfg.manufacturer = "JNOV";
    cfg.mqttEnabled = true;
    cfg.homeAssistantEnabled = true;
    cfg.mqttClientId = "watermeter_esp32";
    
    gCore = new DomoticsCore(cfg);
    
    DLOG_I(LOG_WATER_METER, "Water Meter v%s starting...", WATER_METER_VERSION);
    DLOG_I(LOG_WATER_METER, "Firmware compiled: %s %s", __DATE__, __TIME__);
    DLOG_I(LOG_WATER_METER, "ESP32 Chip ID: %llX", ESP.getEfuseMac());
    
    // Setup hardware - sensor outputs 3.5V/0.2V (ESP32 compatible)
    // GPIO34 is input-only, no need for pinMode but we'll set it for clarity
    pinMode(PULSE_INPUT_PIN, INPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    
    // Log GPIO setup for debugging
    DLOG_I(LOG_WATER_METER, "GPIO setup: PULSE_PIN=%d (INPUT), LED_PIN=%d", PULSE_INPUT_PIN, STATUS_LED_PIN);
    DLOG_I(LOG_WATER_METER, "Sensor signal levels: HIGH=%.1fV, LOW=%.1fV", SIGNAL_HIGH_VOLTAGE, SIGNAL_LOW_VOLTAGE);
    DLOG_I(LOG_WATER_METER, "Initial GPIO state: PULSE_PIN=%d", digitalRead(PULSE_INPUT_PIN));
    // Initialize GPIO state before setting up interrupt to prevent false triggers
    delay(100); // Allow GPIO to stabilize
    int initialState = digitalRead(PULSE_INPUT_PIN);
    DLOG_I(LOG_WATER_METER, "Initial GPIO state: %d", initialState);
    
    // Setup interrupt for pulse detection - FALLING edge when magnet passes (voltage drops)
    attachInterrupt(digitalPinToInterrupt(PULSE_INPUT_PIN), pulseISR, FALLING);
    
    // Start DomoticsCore - this initializes the storage system
    gCore->begin();
    
    // Load saved data AFTER storage system is initialized
    loadFromStorage();
    
    // Initial state loaded
    
    // Setup web endpoints
    gCore->webServer().on("/counter", HTTP_GET, [](AsyncWebServerRequest *request){
        double totalLiters = pulseCount * LITERS_PER_PULSE;
        double totalM3 = totalLiters / 1000.0;
        double yearlyM3 = yearlyLiters / 1000.0;
        
        String html = FPSTR(COUNTER_PAGE_HTML);
        html.replace("%TOTAL_M3%", String(totalM3, 3));
        html.replace("%DAILY_LITERS%", String(dailyLiters));
        html.replace("%YEARLY_M3%", String(yearlyM3, 3));
        html.replace("%PULSE_COUNT%", String((unsigned long)pulseCount));
        request->send(200, "text/html", html);
    });
    
    gCore->webServer().on("/api/counter", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        double totalLiters = pulseCount * LITERS_PER_PULSE;
        double totalM3 = totalLiters / 1000.0;
        double yearlyM3 = yearlyLiters / 1000.0;
        
        doc["pulse_count"] = (unsigned long)pulseCount;
        doc["daily_liters"] = (unsigned long)dailyLiters;
        doc["yearly_liters"] = (unsigned long)yearlyLiters;
        doc["total_liters"] = totalLiters;
        doc["total_m3"] = totalM3;
        doc["yearly_m3"] = yearlyM3;
        doc["liters_per_pulse"] = LITERS_PER_PULSE;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    gCore->webServer().on("/api/counter", HTTP_POST, [](AsyncWebServerRequest *request){
        bool updated = false;
        
        // Accept total liters (user-friendly) and convert to pulse count
        if (request->hasParam("total_liters", true)) {
            double liters = request->getParam("total_liters", true)->value().toDouble();
            pulseCount = (uint64_t)(liters / LITERS_PER_PULSE);
            updated = true;
        }
        
        // Accept daily liters directly
        if (request->hasParam("daily_liters", true)) {
            dailyLiters = request->getParam("daily_liters", true)->value().toInt();
            updated = true;
        }
        
        // Accept yearly liters directly
        if (request->hasParam("yearly_liters", true)) {
            yearlyLiters = request->getParam("yearly_liters", true)->value().toInt();
            updated = true;
        }
        
        // Legacy support for pulse_count (but discouraged)
        if (request->hasParam("pulse_count", true)) {
            pulseCount = request->getParam("pulse_count", true)->value().toInt();
            updated = true;
        }
        
        if (updated) {
            saveToStorage();
            request->send(200, "text/plain", "Counter updated");
        } else {
            request->send(400, "text/plain", "Invalid request - use total_liters, daily_liters, or yearly_liters");
        }
    });

    gCore->webServer().on("/counter", HTTP_POST, [](AsyncWebServerRequest* request){
        if (request->hasParam("value", true)) {
            const AsyncWebParameter* p = request->getParam("value", true);
            unsigned long newTotal = strtoul(p->value().c_str(), nullptr, 10);
            unsigned long newPulseCount = (unsigned long)(newTotal / LITERS_PER_PULSE);
            pulseCount = newPulseCount;
            if (request->hasParam("reset_daily", true)) {
                dailyLiters = 0;
            }
            saveToStorage();
            request->send(200, "text/plain", "Counter updated");
        } else {
            request->send(400, "text/plain", "Missing 'value'");
        }
    });

    gCore->webServer().on("/api/water", HTTP_GET, [](AsyncWebServerRequest* request){
        unsigned long totalLiters = pulseCount * LITERS_PER_PULSE;
        JsonDocument doc;
        doc["total_liters"] = totalLiters;
        doc["daily_liters"] = dailyLiters;
        doc["pulse_count"] = pulseCount;
        doc["status"] = "ok";
        String out; 
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });
    
    gCore->webServer().on("/api/water/reset", HTTP_POST, [](AsyncWebServerRequest* request){
        if (request->hasParam("daily", true)) {
            dailyLiters = 0;
            saveToStorage();
            request->send(200, "text/plain", "Daily counter reset");
            DLOG_I(LOG_WATER_METER, "Daily counter reset via API");
        } else if (request->hasParam("yearly", true)) {
            yearlyLiters = 0;
            saveToStorage();
            request->send(200, "text/plain", "Yearly counter reset");
            DLOG_I(LOG_WATER_METER, "Yearly counter reset via API");
        } else {
            request->send(400, "text/plain", "Invalid request - use daily=1 or yearly=1");
        }
    });
    
    // Start DomoticsCore
    gCore->begin();
    
    // Setup Home Assistant auto-discovery
    if (gCore->isHomeAssistantEnabled()) {
        gCore->getHomeAssistant().publishSensor("total_water", "Total Water Consumption", "m³", "water");
        gCore->getHomeAssistant().publishSensor("daily_water", "Daily Water Consumption", "L", "water");
        gCore->getHomeAssistant().publishSensor("yearly_water", "Yearly Water Consumption", "m³", "water");
        gCore->getHomeAssistant().publishDevice();
        DLOG_I(LOG_HA, "Water meter sensors published to Home Assistant");
    }
    
    DLOG_I(LOG_WATER_METER, "Water Meter ready!");
}

void loop() {
    gCore->loop();
    
    // Clean operation - no debug logging
    
    // Handle ISR flags for logging (avoid logging in interrupt context)
    if (newPulseDetected) {
        // Update daily and yearly counters in main loop (not ISR)
        dailyLiters += LITERS_PER_PULSE;
        yearlyLiters += LITERS_PER_PULSE;
        
        DLOG_I(LOG_SENSOR, "PULSE detected: count=%llu, daily=%llu L, yearly=%llu L (magnet passed)", pulseCount, dailyLiters, yearlyLiters);
        saveToStorage(); // Save immediately to prevent data loss on reboot
        newPulseDetected = false;
    }
    
    if (pulseIgnored) {
        DLOG_W(LOG_SENSOR, "Pulse ignored (debounce): time_diff=%lu ms (threshold=%d ms)", lastIgnoredTimeDiff, PULSE_DEBOUNCE_MS);
        pulseIgnored = false;
    }
    
    static unsigned long lastSave = 0;
    static unsigned long lastPublish = 0;
    
    // Check for daily and yearly resets
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Daily reset at midnight
    if (lastDay == -1) {
        lastDay = timeinfo.tm_mday;
    } else if (timeinfo.tm_mday != lastDay) {
        dailyLiters = 0;
        lastDay = timeinfo.tm_mday;
        DLOG_I(LOG_WATER_METER, "Daily counter reset automatically");
        saveToStorage();
    }
    
    // Yearly reset on January 1st
    if (lastYear == -1) {
        lastYear = timeinfo.tm_year;
    } else if (timeinfo.tm_year != lastYear) {
        yearlyLiters = 0;
        lastYear = timeinfo.tm_year;
        DLOG_I(LOG_WATER_METER, "Yearly counter reset automatically");
        saveToStorage();
    }
    
    // Save data every 30 seconds
    if (millis() - lastSave > 30000) {
        saveToStorage();
        lastSave = millis();
    }
    
    // Publish data every minute
    if (millis() - lastPublish > 60000 && gCore->isMQTTConnected()) {
        double totalLiters = pulseCount * LITERS_PER_PULSE;
        double totalM3 = totalLiters / 1000.0;
        double yearlyM3 = yearlyLiters / 1000.0;
        
        JsonDocument doc;
        doc["total_liters"] = totalLiters;
        doc["daily_liters"] = (unsigned long)dailyLiters;
        doc["yearly_liters"] = (unsigned long)yearlyLiters;
        doc["pulse_count"] = (unsigned long)pulseCount;
        doc["total_m3"] = totalM3;
        doc["yearly_m3"] = yearlyM3;
        doc["status"] = "ok";
        
        String payload;
        serializeJson(doc, payload);
        
        PubSubClient& mqtt = gCore->getMQTTClient();
        mqtt.publish("watermeter/data", payload.c_str());
        
        // Home Assistant state updates
        if (gCore->isHomeAssistantEnabled()) {
            String deviceId = gCore->config().deviceName;
            mqtt.publish(("jnov/" + deviceId + "/total_water/state").c_str(), String(totalM3).c_str());
            mqtt.publish(("jnov/" + deviceId + "/daily_water/state").c_str(), String(dailyLiters).c_str());
            mqtt.publish(("jnov/" + deviceId + "/yearly_water/state").c_str(), String(yearlyM3).c_str());
        }
        
        lastPublish = millis();
        DLOG_D(LOG_WATER_METER, "Published: %s", payload.c_str());
    }
    
    // Status LED blink on pulse
    static unsigned long ledOnTime = 0;
    if (digitalRead(STATUS_LED_PIN) == HIGH && millis() - ledOnTime > 100) {
        digitalWrite(STATUS_LED_PIN, LOW);
    }
    if (pulseCount > 0 && millis() - lastPulseTime < 100) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        ledOnTime = millis();
    }
    
}
