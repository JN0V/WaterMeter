#ifndef WATER_METER_CONFIG_H
#define WATER_METER_CONFIG_H

// Water Meter Configuration
#define WATER_METER_VERSION "0.1.0"

// Hardware Configuration
#define PULSE_INPUT_PIN 2           // GPIO pin for pulse detection (interrupt capable)
#define STATUS_LED_PIN 25           // External LED for status indication (avoid conflict with DomoticsCore)
#define VOLTAGE_DIVIDER_PIN 34      // ADC pin for voltage monitoring (optional)

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

// Voltage Protection (for 24V input)
#define INPUT_VOLTAGE_MAX 30.0      // Maximum expected input voltage
#define INPUT_VOLTAGE_MIN 18.0      // Minimum expected input voltage
#define VOLTAGE_DIVIDER_RATIO 8.2   // Voltage divider ratio (24V -> 3.3V)

// Debug Settings
#ifdef WATER_METER_DEBUG
    #define DEBUG_SERIAL_SPEED 115200
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(format, ...) Serial.printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(format, ...)
#endif

// EEPROM Addresses
#define EEPROM_SIZE 512
#define EEPROM_TOTAL_LITERS_ADDR 0      // 4 bytes for total consumption
#define EEPROM_DAILY_LITERS_ADDR 4      // 4 bytes for daily consumption
#define EEPROM_LAST_SAVE_TIME_ADDR 8    // 4 bytes for last save timestamp
#define EEPROM_CONFIG_VERSION_ADDR 12   // 4 bytes for config version
#define EEPROM_PULSE_COUNT_ADDR 16      // 4 bytes for pulse count

// Error Codes
#define ERROR_NONE 0
#define ERROR_WIFI_CONNECTION 1
#define ERROR_MQTT_CONNECTION 2
#define ERROR_SENSOR_FAULT 3
#define ERROR_VOLTAGE_OUT_OF_RANGE 4
#define ERROR_EEPROM_WRITE 5

#endif // WATER_METER_CONFIG_H
