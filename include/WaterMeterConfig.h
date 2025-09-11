#ifndef WATER_METER_CONFIG_H
#define WATER_METER_CONFIG_H

// Water Meter Configuration
#define WATER_METER_VERSION "0.3.0"

// Hardware Configuration
#define PULSE_INPUT_PIN 34          // GPIO pin for pulse detection (input-only, interrupt capable)
#define STATUS_LED_PIN 25           // External LED for status indication (avoid conflict with DomoticsCore)
// #define VOLTAGE_DIVIDER_PIN 34   // Not needed - sensor has built-in voltage regulation

// Water Meter Settings
#define LITERS_PER_PULSE 1.0        // Volume per pulse in liters
#define PULSE_DEBOUNCE_MS 50        // Debounce time in milliseconds
#define MIN_PULSE_WIDTH_MS 10       // Minimum pulse width to consider valid
#define MAX_PULSE_WIDTH_MS 1000     // Maximum pulse width to consider valid

// Data Storage
#define SAVE_INTERVAL_MS 30000      // Save data every 30 seconds
#define BACKUP_INTERVAL_MS 300000   // Backup data every 5 minutes
#define MAX_DAILY_RECORDS 288       // Store 5-minute intervals for 24 hours

// Network Configuration
#define MQTT_PUBLISH_INTERVAL 60000     // Publish data every minute
#define NTP_UPDATE_INTERVAL 3600000     // Update time every hour

// Signal Levels (sensor with built-in voltage regulation)
#define SIGNAL_HIGH_VOLTAGE 3.5     // High signal level (pulse active)
#define SIGNAL_LOW_VOLTAGE 0.2      // Low signal level (no pulse)
#define SIGNAL_THRESHOLD 1.8        // GPIO threshold for reliable detection

// Logging Configuration - Using DomoticsCore logging system
// Log levels controlled by CORE_DEBUG_LEVEL build flag:
// 0=None, 1=Error, 2=Warning, 3=Info, 4=Debug, 5=Verbose
// Default level is 3 (Info)

// Water Meter specific log tags
#define LOG_WATER_METER "WATER"
#define LOG_SENSOR      "SENSOR"
#define LOG_COUNTER     "COUNTER"
#define LOG_API         "API"

// Web Interface Settings
#define WEB_COUNTER_PATH "/counter"
#define API_COUNTER_PATH "/api/counter"
#define MQTT_COMMAND_TOPIC "watermeter/command"
#define MQTT_RESPONSE_TOPIC "watermeter/response"

// Error Codes
#define ERROR_NONE 0
#define ERROR_WIFI_CONNECTION 1
#define ERROR_MQTT_CONNECTION 2
#define ERROR_SENSOR_FAULT 3
#define ERROR_SIGNAL_OUT_OF_RANGE 4
#define ERROR_STORAGE_WRITE 5
#define ERROR_INVALID_COUNTER_VALUE 6

#endif // WATER_METER_CONFIG_H
