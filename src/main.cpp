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
void setupWebInterface();
void handleCounterWebPage(AsyncWebServerRequest *request);
void handleCounterAPI(AsyncWebServerRequest *request);
void handleCounterSet(AsyncWebServerRequest *request);
void handleCounterSetBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleMQTTCommand(String topic, String message);
bool setCounterValue(unsigned long newValue, const String& source);

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
    
    // Setup web interface for counter management
    setupWebInterface();
    
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
    
    // Check if EEPROM is initialized
    unsigned long magicNumber;
    EEPROM.get(EEPROM_MAGIC_ADDR, magicNumber);
    
    if (magicNumber != EEPROM_MAGIC_NUMBER) {
        DEBUG_PRINTLN("EEPROM not initialized, setting default values...");
        
        // First time initialization - set all values to zero
        totalLiters = 0;
        dailyLiters = 0;
        pulseCount = 0;
        
        // Write magic number and default values to EEPROM
        EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_NUMBER);
        EEPROM.put(EEPROM_TOTAL_LITERS_ADDR, totalLiters);
        EEPROM.put(EEPROM_DAILY_LITERS_ADDR, dailyLiters);
        EEPROM.put(EEPROM_PULSE_COUNT_ADDR, pulseCount);
        EEPROM.put(EEPROM_CONFIG_VERSION_ADDR, (unsigned long)1);
        EEPROM.commit();
        
        DEBUG_PRINTLN("EEPROM initialized with default values");
    } else {
        // Read existing values
        EEPROM.get(EEPROM_TOTAL_LITERS_ADDR, totalLiters);
        EEPROM.get(EEPROM_DAILY_LITERS_ADDR, dailyLiters);
        EEPROM.get(EEPROM_PULSE_COUNT_ADDR, pulseCount);
        
        // Validate data and reset if corrupted
        if (totalLiters > 999999999) {
            DEBUG_PRINTLN("Total liters corrupted, resetting to 0");
            totalLiters = 0;
        }
        if (dailyLiters > 999999) {
            DEBUG_PRINTLN("Daily liters corrupted, resetting to 0");
            dailyLiters = 0;
        }
        if (pulseCount > 999999999) {
            DEBUG_PRINTLN("Pulse count corrupted, resetting to 0");
            pulseCount = 0;
        }
    }
    
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
    
    // Save to EEPROM with magic number
    EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_NUMBER);
    EEPROM.put(EEPROM_TOTAL_LITERS_ADDR, totalLiters);
    EEPROM.put(EEPROM_DAILY_LITERS_ADDR, dailyLiters);
    EEPROM.put(EEPROM_PULSE_COUNT_ADDR, pulseCount);
    EEPROM.put(EEPROM_LAST_SAVE_TIME_ADDR, millis());
    EEPROM.commit();
    
    DEBUG_PRINTF("Saved - Total: %lu L, Daily: %lu L, Pulses: %lu\n", 
                 totalLiters, dailyLiters, pulseCount);
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

void setupWebInterface() {
    AsyncWebServer& server = domotics.webServer();
    
    // Web interface page
    server.on(WEB_COUNTER_PATH, HTTP_GET, handleCounterWebPage);
    
    // API endpoints
    server.on(API_COUNTER_PATH, HTTP_GET, handleCounterAPI);
    server.on(API_COUNTER_PATH, HTTP_POST, handleCounterSet, NULL, handleCounterSetBody);
    
    DEBUG_PRINTLN("Web interface setup complete");
}

void handleCounterWebPage(AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><title>Water Meter Counter Management</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #2c3e50; text-align: center; }";
    html += ".info-box { background: #e8f4fd; padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += ".form-group { margin: 15px 0; }";
    html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
    html += "input[type=\"number\"] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; font-size: 16px; }";
    html += "button { background: #3498db; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 5px; }";
    html += "button:hover { background: #2980b9; }";
    html += ".danger { background: #e74c3c; } .danger:hover { background: #c0392b; }";
    html += ".success { background: #27ae60; } .success:hover { background: #229954; }";
    html += ".status { padding: 10px; border-radius: 5px; margin: 10px 0; }";
    html += ".error { background: #ffebee; color: #c62828; border: 1px solid #ffcdd2; }";
    html += ".ok { background: #e8f5e8; color: #2e7d32; border: 1px solid #c8e6c9; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>🚰 Water Meter Counter</h1>";
    html += "<div class=\"info-box\">";
    html += "<h3>Current Values</h3>";
    html += "<p><strong>Total Consumption:</strong> <span id=\"totalLiters\">Loading...</span> L</p>";
    html += "<p><strong>Daily Consumption:</strong> <span id=\"dailyLiters\">Loading...</span> L</p>";
    html += "<p><strong>Pulse Count:</strong> <span id=\"pulseCount\">Loading...</span></p>";
    html += "<p><strong>Flow Rate:</strong> <span id=\"flowRate\">Loading...</span> L/min</p>";
    html += "</div>";
    html += "<div class=\"form-group\">";
    html += "<label for=\"newValue\">Set Counter Value (Liters):</label>";
    html += "<input type=\"number\" id=\"newValue\" placeholder=\"Enter new counter value\" min=\"0\" step=\"0.1\">";
    html += "</div>";
    html += "<button onclick=\"setCounter()\">Set Counter Value</button>";
    html += "<button onclick=\"resetDaily()\" class=\"success\">Reset Daily Counter</button>";
    html += "<button onclick=\"resetAll()\" class=\"danger\">Reset All Counters</button>";
    html += "<button onclick=\"refreshData()\">Refresh Data</button>";
    html += "<div id=\"status\"></div>";
    html += "<div class=\"info-box\">";
    html += "<h3>API Usage</h3>";
    html += "<p><strong>GET</strong> /api/counter - Get current values</p>";
    html += "<p><strong>POST</strong> /api/counter - Set counter value</p>";
    html += "<p>Body: {\"value\": 1234.5, \"reset_daily\": false}</p>";
    html += "</div></div>";
    
    // JavaScript functions
    html += "<script>";
    html += "function refreshData() {";
    html += "fetch('/api/counter').then(response => response.json()).then(data => {";
    html += "document.getElementById('totalLiters').textContent = data.total_liters;";
    html += "document.getElementById('dailyLiters').textContent = data.daily_liters;";
    html += "document.getElementById('pulseCount').textContent = data.pulse_count;";
    html += "document.getElementById('flowRate').textContent = data.flow_rate.toFixed(2);";
    html += "}).catch(error => showStatus('Error loading data: ' + error, 'error'));";
    html += "}";
    
    html += "function setCounter() {";
    html += "const value = parseFloat(document.getElementById('newValue').value);";
    html += "if (isNaN(value) || value < 0) { showStatus('Please enter a valid positive number', 'error'); return; }";
    html += "fetch('/api/counter', { method: 'POST', headers: {'Content-Type': 'application/json'}, ";
    html += "body: JSON.stringify({value: value, reset_daily: false}) })";
    html += ".then(response => response.json()).then(data => {";
    html += "if (data.success) { showStatus('Counter set to ' + value + ' L successfully', 'ok'); refreshData(); }";
    html += "else { showStatus('Error: ' + data.message, 'error'); }";
    html += "}).catch(error => showStatus('Error: ' + error, 'error'));";
    html += "}";
    
    html += "function resetDaily() {";
    html += "if (!confirm('Reset daily counter to 0?')) return;";
    html += "fetch('/api/counter', { method: 'POST', headers: {'Content-Type': 'application/json'}, ";
    html += "body: JSON.stringify({reset_daily: true}) })";
    html += ".then(response => response.json()).then(data => {";
    html += "if (data.success) { showStatus('Daily counter reset successfully', 'ok'); refreshData(); }";
    html += "else { showStatus('Error: ' + data.message, 'error'); }";
    html += "}).catch(error => showStatus('Error: ' + error, 'error'));";
    html += "}";
    
    html += "function resetAll() {";
    html += "if (!confirm('Reset ALL counters to 0? This cannot be undone!')) return;";
    html += "fetch('/api/counter', { method: 'POST', headers: {'Content-Type': 'application/json'}, ";
    html += "body: JSON.stringify({value: 0, reset_daily: true}) })";
    html += ".then(response => response.json()).then(data => {";
    html += "if (data.success) { showStatus('All counters reset successfully', 'ok'); refreshData(); }";
    html += "else { showStatus('Error: ' + data.message, 'error'); }";
    html += "}).catch(error => showStatus('Error: ' + error, 'error'));";
    html += "}";
    
    html += "function showStatus(message, type) {";
    html += "const status = document.getElementById('status');";
    html += "status.innerHTML = '<div class=\"status ' + type + '\">' + message + '</div>';";
    html += "setTimeout(() => status.innerHTML = '', 5000);";
    html += "}";
    
    html += "refreshData(); setInterval(refreshData, 30000);";
    html += "</script></body></html>";
    
    request->send(200, "text/html", html);
}

void handleCounterAPI(AsyncWebServerRequest *request) {
    float flowRate = calculateFlowRate();
    
    String json = "{";
    json += "\"total_liters\":" + String(totalLiters) + ",";
    json += "\"daily_liters\":" + String(dailyLiters) + ",";
    json += "\"pulse_count\":" + String(pulseCount) + ",";
    json += "\"flow_rate\":" + String(flowRate, 2) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"version\":\"" + String(WATER_METER_VERSION) + "\"";
    json += "}";
    
    request->send(200, "application/json", json);
}

// Global variable to store request body
String requestBody = "";

void handleCounterSetBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Accumulate body data
    if (index == 0) {
        requestBody = "";
    }
    
    for (size_t i = 0; i < len; i++) {
        requestBody += (char)data[i];
    }
}

void handleCounterSet(AsyncWebServerRequest *request) {
    String response = "{\"success\":false,\"message\":\"Invalid request\"}";
    
    if (requestBody.length() > 0) {
        DEBUG_PRINTLN("Received JSON body: " + requestBody);
        
        // Simple JSON parsing for our specific format
        int valueIndex = requestBody.indexOf("\"value\":");
        int resetIndex = requestBody.indexOf("\"reset_daily\":");
        
        bool valueProcessed = false;
        bool resetProcessed = false;
        
        if (valueIndex != -1) {
            int valueStart = requestBody.indexOf(":", valueIndex) + 1;
            int valueEnd = requestBody.indexOf(",", valueStart);
            if (valueEnd == -1) valueEnd = requestBody.indexOf("}", valueStart);
            
            String valueStr = requestBody.substring(valueStart, valueEnd);
            valueStr.trim();
            
            float newValue = valueStr.toFloat();
            DEBUG_PRINTLN("Parsed value: " + String(newValue));
            
            if (newValue >= 0 && newValue <= 999999999) {
                if (setCounterValue((unsigned long)newValue, "Web API")) {
                    response = "{\"success\":true,\"message\":\"Counter set to " + String(newValue) + " L\"}";
                    valueProcessed = true;
                } else {
                    response = "{\"success\":false,\"message\":\"Failed to save counter value\"}";
                }
            } else {
                response = "{\"success\":false,\"message\":\"Invalid counter value\"}";
            }
        }
        
        if (resetIndex != -1) {
            int resetStart = requestBody.indexOf(":", resetIndex) + 1;
            int resetEnd = requestBody.indexOf("}", resetStart);
            if (resetEnd == -1) resetEnd = requestBody.length();
            
            String resetStr = requestBody.substring(resetStart, resetEnd);
            resetStr.trim();
            resetStr.replace(",", "");
            DEBUG_PRINTLN("Parsed reset_daily: " + resetStr);
            
            if (resetStr.startsWith("true")) {
                dailyLiters = 0;
                saveDataToEEPROM();
                resetProcessed = true;
                
                if (valueProcessed) {
                    response = "{\"success\":true,\"message\":\"Counter set and daily reset\"}";
                } else {
                    response = "{\"success\":true,\"message\":\"Daily counter reset\"}";
                }
            }
        }
        
        if (!valueProcessed && !resetProcessed) {
            response = "{\"success\":false,\"message\":\"No valid operation found\"}";
        }
        
        // Clear the body for next request
        requestBody = "";
    } else {
        response = "{\"success\":false,\"message\":\"No request body found\"}";
    }
    
    request->send(200, "application/json", response);
}

void handleMQTTCommand(String topic, String message) {
    if (topic == MQTT_COMMAND_TOPIC) {
        PubSubClient& mqtt = domotics.getMQTTClient();
        String response = "";
        
        if (message.startsWith("SET:")) {
            String valueStr = message.substring(4);
            float newValue = valueStr.toFloat();
            
            if (newValue >= 0 && newValue <= 999999999) {
                if (setCounterValue((unsigned long)newValue, "MQTT")) {
                    response = "SUCCESS:Counter set to " + String(newValue) + " L";
                } else {
                    response = "ERROR:Failed to save counter value";
                }
            } else {
                response = "ERROR:Invalid counter value";
            }
        }
        else if (message == "RESET_DAILY") {
            dailyLiters = 0;
            saveDataToEEPROM();
            response = "SUCCESS:Daily counter reset";
        }
        else if (message == "RESET_ALL") {
            if (setCounterValue(0, "MQTT")) {
                dailyLiters = 0;
                saveDataToEEPROM();
                response = "SUCCESS:All counters reset";
            } else {
                response = "ERROR:Failed to reset counters";
            }
        }
        else if (message == "STATUS") {
            float flowRate = calculateFlowRate();
            unsigned long currentTotalLiters = pulseCount * LITERS_PER_PULSE;
            response = "STATUS:Total=" + String(currentTotalLiters) + "L,Daily=" + String(dailyLiters) + "L,Flow=" + String(flowRate, 2) + "L/min";
        }
        else {
            response = "ERROR:Unknown command";
        }
        
        if (response.length() > 0 && mqtt.connected()) {
            mqtt.publish(MQTT_RESPONSE_TOPIC, response.c_str());
        }
    }
}


bool setCounterValue(unsigned long newValue, const String& source) {
    if (newValue > 999999999) {
        DEBUG_PRINTLN("Counter value too large: " + String(newValue));
        return false;
    }
    
    // Calculate new pulse count based on desired liter value
    unsigned long newPulseCount = newValue / LITERS_PER_PULSE;
    
    // Update values
    pulseCount = newPulseCount;
    totalLiters = newValue;
    
    // Update EEPROM with magic number to ensure proper initialization
    EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_NUMBER);
    EEPROM.put(EEPROM_TOTAL_LITERS_ADDR, totalLiters);
    EEPROM.put(EEPROM_PULSE_COUNT_ADDR, pulseCount);
    EEPROM.put(EEPROM_DAILY_LITERS_ADDR, dailyLiters);
    EEPROM.commit();
    
    DEBUG_PRINTF("Counter set to %lu L (%lu pulses) by %s\n", newValue, newPulseCount, source.c_str());
    
    // Publish update via MQTT
    PubSubClient& mqtt = domotics.getMQTTClient();
    if (mqtt.connected()) {
        String message = "Counter updated to " + String(newValue) + " L by " + source;
        mqtt.publish("watermeter/counter_update", message.c_str());
    }
    
    return true;
}
