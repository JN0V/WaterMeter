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

// Water meter variables
volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
unsigned long dailyLiters = 0;
static int lastDay = -1;

// ISR flags for logging (avoid logging in interrupt context)
volatile bool newPulseDetected = false;
volatile bool pulseIgnored = false;
volatile unsigned long lastIgnoredTimeDiff = 0;

// DomoticsCore storage
void loadFromStorage() {
    pulseCount = gCore->storage().getULong("pulse_count", 0);
    dailyLiters = gCore->storage().getULong("daily_liters", 0);
    DLOG_I(LOG_WATER_METER, "Loaded: %lu pulses, %lu L daily", pulseCount, dailyLiters);
}

void saveToStorage() {
    gCore->storage().putULong("pulse_count", pulseCount);
    gCore->storage().putULong("daily_liters", dailyLiters);
}

// Interrupt for pulse detection
void IRAM_ATTR pulseISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastPulseTime > PULSE_DEBOUNCE_MS) {
        pulseCount++;
        lastPulseTime = currentTime;
        newPulseDetected = true;
    } else {
        pulseIgnored = true;
        lastIgnoredTimeDiff = currentTime - lastPulseTime;
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
    
    // Setup hardware
    pinMode(PULSE_INPUT_PIN, INPUT_PULLDOWN);
    pinMode(STATUS_LED_PIN, OUTPUT);
    
    // Log GPIO setup for debugging
    DLOG_I(LOG_WATER_METER, "GPIO setup: PULSE_PIN=%d (INPUT_PULLDOWN), LED_PIN=%d", PULSE_INPUT_PIN, STATUS_LED_PIN);
    DLOG_I(LOG_WATER_METER, "Initial GPIO state: PULSE_PIN=%d", digitalRead(PULSE_INPUT_PIN));
    
    attachInterrupt(digitalPinToInterrupt(PULSE_INPUT_PIN), pulseISR, RISING);
    
    // Load saved data
    loadFromStorage();
    
    // Log initial state for debugging
    DLOG_I(LOG_WATER_METER, "Initial state after load: pulseCount=%lu, dailyLiters=%lu", pulseCount, dailyLiters);
    
    // Minimal web page to view and override counter
    gCore->webServer().on("/counter", HTTP_GET, [](AsyncWebServerRequest* request){
        unsigned long totalLiters = pulseCount * LITERS_PER_PULSE;
        String html = FPSTR(COUNTER_PAGE_HTML);
        html.replace("%TOTAL_LITERS%", String(totalLiters));
        html.replace("%DAILY_LITERS%", String(dailyLiters));
        html.replace("%PULSE_COUNT%", String(pulseCount));
        request->send(200, "text/html", html);
    });

    gCore->webServer().on("/counter", HTTP_POST, [](AsyncWebServerRequest* request){
        // Expect application/x-www-form-urlencoded with fields: value, reset_daily(optional)
        if (request->hasParam("value", true)) {
            const AsyncWebParameter* p = request->getParam("value", true);
            unsigned long newTotal = strtoul(p->value().c_str(), nullptr, 10);
            unsigned long newPulseCount = (unsigned long)(newTotal / LITERS_PER_PULSE);
            pulseCount = newPulseCount;
            if (request->hasParam("reset_daily", true)) {
                dailyLiters = 0;
            }
            saveToStorage();
            DLOG_I(LOG_WATER_METER, "Counter updated via Web UI: total=%lu L (pulses=%lu)", newTotal, newPulseCount);
            request->redirect("/counter");
        } else {
            request->send(400, "text/plain", "Missing 'value'");
        }
    });

    // Simple API endpoints
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
        } else {
            request->send(400, "text/plain", "Invalid request");
        }
    });
    
    // Start DomoticsCore
    gCore->begin();
    
    // Setup Home Assistant auto-discovery
    if (gCore->isHomeAssistantEnabled()) {
        gCore->getHomeAssistant().publishSensor("total_water", "Total Water Consumption", "L", "water");
        gCore->getHomeAssistant().publishSensor("daily_water", "Daily Water Consumption", "L", "water");
        gCore->getHomeAssistant().publishDevice();
        DLOG_I(LOG_HA, "Water meter sensors published to Home Assistant");
    }
    
    DLOG_I(LOG_WATER_METER, "Water Meter ready!");
}

void loop() {
    if (gCore) gCore->loop();
    
    static unsigned long lastSave = 0;
    static unsigned long lastPublish = 0;
    
    // Save data every 30 seconds
    if (millis() - lastSave > 30000) {
        saveToStorage();
        lastSave = millis();
    }
    
    // Publish data every minute
    if (millis() - lastPublish > 60000 && gCore->isMQTTConnected()) {
        unsigned long totalLiters = pulseCount * LITERS_PER_PULSE;
        
        JsonDocument doc;
        doc["total_liters"] = totalLiters;
        doc["daily_liters"] = dailyLiters;
        doc["pulse_count"] = pulseCount;
        doc["status"] = "ok";
        
        String payload;
        serializeJson(doc, payload);
        
        PubSubClient& mqtt = gCore->getMQTTClient();
        mqtt.publish("watermeter/data", payload.c_str());
        
        // Home Assistant state updates
        if (gCore->isHomeAssistantEnabled()) {
            String deviceId = gCore->config().deviceName;
            mqtt.publish(("jnov/" + deviceId + "/total_water/state").c_str(), String(totalLiters).c_str());
            mqtt.publish(("jnov/" + deviceId + "/daily_water/state").c_str(), String(dailyLiters).c_str());
        }
        
        lastPublish = millis();
        DLOG_D(LOG_WATER_METER, "Published: %s", payload.c_str());
    }
    
    // Reset daily counter at midnight (simplified)
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    if (lastDay == -1) {
        lastDay = timeinfo.tm_mday;
    } else if (timeinfo.tm_mday != lastDay) {
        DLOG_I(LOG_WATER_METER, "New day - resetting daily counter");
        dailyLiters = 0;
        lastDay = timeinfo.tm_mday;
        saveToStorage();
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
    
    // Log pulse events
    if (newPulseDetected) {
        DLOG_I(LOG_SENSOR, "PULSE detected: count=%lu, time=%lu ms", pulseCount, lastPulseTime);
        newPulseDetected = false;
    }
    if (pulseIgnored) {
        DLOG_D(LOG_SENSOR, "Pulse ignored (debounce): time_diff=%lu ms", lastIgnoredTimeDiff);
        pulseIgnored = false;
    }
}
