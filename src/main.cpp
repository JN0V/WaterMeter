#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <time.h>
#include <DomoticsCore/DomoticsCore.h>
#include "WaterMeterConfig.h"

// Global variables
volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
unsigned long totalLiters = 0;
unsigned long dailyLiters = 0;
unsigned long lastSaveTime = 0;
unsigned long lastPublishTime = 0;
unsigned long lastBackupTime = 0;
bool sensorFault = false;
float inputVoltage = 0.0;

// DomoticsCore instance
DomoticsCore domotics;

// Function declarations
void loadDataFromEEPROM();
void saveDataToEEPROM();
void publishWaterMeterData();
void monitorInputVoltage();
void checkSensorHealth();
float calculateFlowRate();
void resetDailyCounterIfNeeded();
void backupData();

// Interrupt service routine for pulse detection
void IRAM_ATTR pulseISR() {
    unsigned long currentTime = millis();
    
    // Debounce check
    if (currentTime - lastPulseTime > PULSE_DEBOUNCE_MS) {
        pulseCount++;
        lastPulseTime = currentTime;
        
        DEBUG_PRINTLN("Pulse detected!");
        
        // Blink status LED (non-blocking)
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
}

void setup() {
    Serial.begin(DEBUG_SERIAL_SPEED);
    DEBUG_PRINTLN("Water Meter System Starting...");
    DEBUG_PRINTF("Version: %s\n", WATER_METER_VERSION);
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Load saved data from EEPROM
    loadDataFromEEPROM();
    
    // Initialize GPIO pins
    pinMode(PULSE_INPUT_PIN, INPUT_PULLDOWN);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(VOLTAGE_DIVIDER_PIN, INPUT);
    
    // Attach interrupt for pulse detection
    attachInterrupt(digitalPinToInterrupt(PULSE_INPUT_PIN), pulseISR, RISING);
    
    // Initialize DomoticsCore
    domotics.begin();
    
    // DomoticsCore handles WiFi, MQTT, and OTA automatically
    // Configuration is done via web interface at 192.168.4.1
    
    DEBUG_PRINTLN("WiFi connected!");
    DEBUG_PRINTF("IP Address: %s\n", WiFi.localIP().toString().c_str());
    
    // Initialize NTP for time synchronization
    configTime(0, 0, "pool.ntp.org");
    
    // Publish initial status
    publishWaterMeterData();
    
    DEBUG_PRINTLN("Water Meter System Ready!");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle DomoticsCore tasks
    domotics.loop();
    
    // Monitor input voltage
    monitorInputVoltage();
    
    // Check for sensor faults
    checkSensorHealth();
    
    // Save data periodically
    if (currentTime - lastSaveTime > SAVE_INTERVAL_MS) {
        saveDataToEEPROM();
        lastSaveTime = currentTime;
    }
    
    // Publish data via MQTT
    if (currentTime - lastPublishTime > MQTT_PUBLISH_INTERVAL) {
        publishWaterMeterData();
        lastPublishTime = currentTime;
    }
    
    // Backup data
    if (currentTime - lastBackupTime > BACKUP_INTERVAL_MS) {
        backupData();
        lastBackupTime = currentTime;
    }
    
    // Reset daily counter at midnight
    resetDailyCounterIfNeeded();
    
    // Handle LED blinking for pulse indication
    static unsigned long ledOnTime = 0;
    static bool ledState = false;
    
    if (ledState && millis() - ledOnTime > 100) {
        digitalWrite(STATUS_LED_PIN, LOW);
        ledState = false;
    }
    
    // Turn on LED when pulse detected
    if (digitalRead(STATUS_LED_PIN) == HIGH && !ledState) {
        ledOnTime = millis();
        ledState = true;
    }
    
    delay(100); // Small delay to prevent watchdog issues
}

void loadDataFromEEPROM() {
    DEBUG_PRINTLN("Loading data from EEPROM...");
    
    // Read total liters
    EEPROM.get(EEPROM_TOTAL_LITERS_ADDR, totalLiters);
    
    // Read daily liters
    EEPROM.get(EEPROM_DAILY_LITERS_ADDR, dailyLiters);
    
    // Read pulse count
    EEPROM.get(EEPROM_PULSE_COUNT_ADDR, pulseCount);
    
    // Validate data
    if (totalLiters > 999999999) totalLiters = 0;
    if (dailyLiters > 999999) dailyLiters = 0;
    if (pulseCount > 999999999) pulseCount = 0;
    
    DEBUG_PRINTF("Loaded - Total: %lu L, Daily: %lu L, Pulses: %lu\n", 
                 totalLiters, dailyLiters, pulseCount);
}

void saveDataToEEPROM() {
    DEBUG_PRINTLN("Saving data to EEPROM...");
    
    // Calculate current consumption
    unsigned long currentTotalLiters = pulseCount * LITERS_PER_PULSE;
    unsigned long newLiters = currentTotalLiters - totalLiters;
    
    totalLiters = currentTotalLiters;
    dailyLiters += newLiters;
    
    // Save to EEPROM
    EEPROM.put(EEPROM_TOTAL_LITERS_ADDR, totalLiters);
    EEPROM.put(EEPROM_DAILY_LITERS_ADDR, dailyLiters);
    EEPROM.put(EEPROM_PULSE_COUNT_ADDR, pulseCount);
    EEPROM.put(EEPROM_LAST_SAVE_TIME_ADDR, millis());
    
    EEPROM.commit();
    
    DEBUG_PRINTF("Saved - Total: %lu L, Daily: %lu L, New: %lu L\n", 
                 totalLiters, dailyLiters, newLiters);
}

void publishWaterMeterData() {
    if (!domotics.isMQTTConnected()) {
        DEBUG_PRINTLN("MQTT not connected, skipping publish");
        return;
    }
    
    DEBUG_PRINTLN("Publishing water meter data...");
    
    // Calculate current values
    unsigned long currentTotalLiters = pulseCount * LITERS_PER_PULSE;
    float flowRate = calculateFlowRate();
    
    // Create JSON payload
    String payload = "{";
    payload += "\"total_liters\":" + String(currentTotalLiters) + ",";
    payload += "\"daily_liters\":" + String(dailyLiters) + ",";
    payload += "\"pulse_count\":" + String(pulseCount) + ",";
    payload += "\"flow_rate\":" + String(flowRate, 2) + ",";
    payload += "\"input_voltage\":" + String(inputVoltage, 1) + ",";
    payload += "\"sensor_fault\":" + String(sensorFault ? "true" : "false") + ",";
    payload += "\"uptime\":" + String(millis()) + ",";
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"wifi_rssi\":" + String(WiFi.RSSI());
    payload += "}";
    
    // Publish to MQTT using DomoticsCore's MQTT client
    PubSubClient& mqtt = domotics.getMQTTClient();
    if (mqtt.connected()) {
        mqtt.publish("watermeter/data", payload.c_str());
        mqtt.publish("watermeter/total", String(currentTotalLiters).c_str());
        mqtt.publish("watermeter/daily", String(dailyLiters).c_str());
        mqtt.publish("watermeter/status", sensorFault ? "fault" : "ok");
    }
    
    DEBUG_PRINTF("Published: %s\n", payload.c_str());
}

void monitorInputVoltage() {
    // Read ADC value and convert to voltage
    int adcValue = analogRead(VOLTAGE_DIVIDER_PIN);
    inputVoltage = (adcValue / 4095.0) * 3.3 * VOLTAGE_DIVIDER_RATIO;
    
    // Check if voltage is within acceptable range
    if (inputVoltage < INPUT_VOLTAGE_MIN || inputVoltage > INPUT_VOLTAGE_MAX) {
        if (!sensorFault) {
            DEBUG_PRINTF("Voltage out of range: %.1fV\n", inputVoltage);
            sensorFault = true;
        }
    } else {
        if (sensorFault) {
            DEBUG_PRINTLN("Voltage back to normal range");
            sensorFault = false;
        }
    }
}

void checkSensorHealth() {
    unsigned long timeSinceLastPulse = millis() - lastPulseTime;
    
    // If no pulse for more than 24 hours, it might indicate a sensor issue
    // (This is configurable based on expected water usage patterns)
    if (timeSinceLastPulse > 86400000 && !sensorFault) { // 24 hours
        DEBUG_PRINTLN("Warning: No pulses detected for 24 hours");
        // This is just a warning, not necessarily a fault
    }
}

float calculateFlowRate() {
    // Calculate flow rate based on recent pulses
    // This is a simplified calculation - you might want to implement
    // a more sophisticated moving average
    static unsigned long lastFlowCalcTime = 0;
    static unsigned long lastFlowPulseCount = 0;
    
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastFlowCalcTime;
    
    if (timeDiff >= 60000) { // Calculate every minute
        unsigned long pulseDiff = pulseCount - lastFlowPulseCount;
        float flowRate = (pulseDiff * LITERS_PER_PULSE * 60000.0) / timeDiff; // L/min
        
        lastFlowCalcTime = currentTime;
        lastFlowPulseCount = pulseCount;
        
        return flowRate;
    }
    
    return 0.0;
}

void resetDailyCounterIfNeeded() {
    static int lastDay = -1;
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    if (lastDay == -1) {
        lastDay = timeinfo.tm_mday;
        return;
    }
    
    if (timeinfo.tm_mday != lastDay) {
        DEBUG_PRINTLN("New day detected, resetting daily counter");
        dailyLiters = 0;
        lastDay = timeinfo.tm_mday;
        saveDataToEEPROM();
    }
}

void backupData() {
    DEBUG_PRINTLN("Creating data backup...");
    
    // Publish backup data to a different MQTT topic
    String backupPayload = "{";
    backupPayload += "\"timestamp\":" + String(millis()) + ",";
    backupPayload += "\"total_liters\":" + String(totalLiters) + ",";
    backupPayload += "\"daily_liters\":" + String(dailyLiters) + ",";
    backupPayload += "\"pulse_count\":" + String(pulseCount);
    backupPayload += "}";
    
    PubSubClient& mqtt = domotics.getMQTTClient();
    if (mqtt.connected()) {
        mqtt.publish("watermeter/backup", backupPayload.c_str());
    }
}
