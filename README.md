# Water Meter Monitor

A smart water consumption monitoring system using ESP32 and magnetic pulse detection for accurate water usage tracking.

## Features

- **Pulse Detection**: Accurate water consumption measurement from 24V pulse signals
- **Circuit Protection**: Safe voltage conversion with comprehensive protection circuits
- **WiFi Connectivity**: Automatic connection and reconnection handling via DomoticsCore
- **MQTT Integration**: Real-time data publishing to Home Assistant and other systems
- **Data Persistence**: EEPROM storage with automatic backup and recovery
- **OTA Updates**: Over-the-air firmware updates via DomoticsCore
- **Flow Rate Calculation**: Real-time water flow monitoring
- **Daily Reset**: Automatic daily consumption counter reset at midnight
- **Status LED**: External LED indicator for system status
- **Voltage Monitoring**: Input voltage monitoring for fault detection
- **Debug Interface**: Comprehensive serial debugging output
- **Counter Management**: Multiple interfaces for initializing/correcting counter values:
  - **Web Interface**: User-friendly HTML interface at `/counter`
  - **REST API**: JSON endpoints for integration at `/api/counter`
  - **MQTT Commands**: Remote control via `watermeter/command` topic

## Hardware Requirements

### Components
- ESP32 Development Board
- Breadboard and jumper wires
- Water meter with magnetic pulse output (1 pulse = 1 liter)
- Circuit protection components (see [Circuit Protection Guide](docs/circuit_protection.md))

### Protection Circuit Options

#### Option 1: Voltage Divider (Simple)
- 22kΩ resistor (1/4W)
- 3.3kΩ resistor (1/4W)
- 3.6V Zener diode (500mW)
- 100nF ceramic capacitor (optional)

#### Option 2: Optocoupler (Isolated)
- PC817 optocoupler
- 1kΩ resistor (1/4W)
- 10kΩ resistor (1/4W)

## Software Requirements

- [PlatformIO](https://platformio.org/) IDE
- [DomoticsCore Library](https://github.com/JN0V/DomoticsCore.git)

## Installation

### 1. Clone Repository
```bash
git clone https://github.com/JN0V/WaterMeter.git
cd WaterMeter
```

### 2. Hardware Setup
1. Build the protection circuit according to [Circuit Protection Guide](docs/circuit_protection.md)
2. Connect the 24V pulse output from your water meter to the protection circuit input
3. Connect the protection circuit output to ESP32 GPIO2
4. Connect ESP32 power and ground to your breadboard rails

### 3. Software Configuration

#### Configure DomoticsCore
The project uses your DomoticsCore library for WiFi, MQTT, and OTA functionality. 

**Configuration Options:**

1. **Access Point Mode (Default)**: On first boot, DomoticsCore creates a WiFi Access Point for easy configuration:
   - Connect to AP: `WaterMeter-XXXXXX` (where XXXXXX is device ID)
   - Open browser to: `192.168.4.1`
   - Configure WiFi credentials and MQTT settings via web interface

2. **Manual Configuration** (optional): You can also configure programmatically:
```cpp
// WiFi Configuration
domotics.setWiFiCredentials("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");

// MQTT Configuration  
domotics.setMQTTBroker("mqtt.broker.address", 1883);
domotics.setMQTTCredentials("username", "password");
domotics.setMQTTClientId("watermeter_esp32");
```

#### Customize Settings
Edit `include/WaterMeterConfig.h` to match your setup:

```cpp
#define PULSE_INPUT_PIN 2           // GPIO pin for pulse detection
#define LITERS_PER_PULSE 1.0        // Volume per pulse (adjust if needed)
#define PULSE_DEBOUNCE_MS 50        // Debounce time
#define VOLTAGE_DIVIDER_RATIO 8.2   // Adjust based on your circuit
```

### 4. Build and Upload

```bash
# Using PlatformIO CLI
pio run --target upload

# Or using PlatformIO IDE
# Click the upload button in the IDE
```

### 5. Monitor Serial Output

```bash
pio device monitor
```

## Usage

1. **Power on the ESP32** - The system will start automatically
2. **Connect to WiFi** - Use DomoticsCore's access point mode at `192.168.4.1` for initial setup
3. **Configure MQTT** - Set your MQTT broker details in the web interface
4. **Initialize Counter** - Set the initial counter value using one of the management interfaces
5. **Monitor consumption** - Data will be published to MQTT topics automatically
6. **Check status** - Monitor the external LED on GPIO25 for system status

### Counter Management Interfaces

#### Web Interface
Access the counter management page at: `http://[ESP32_IP]/counter`
- View current consumption values
- Set counter to specific value
- Reset daily counter
- Reset all counters

#### REST API
- **GET** `/api/counter` - Get current values (JSON)
- **POST** `/api/counter` - Set counter value
  ```json
  {"value": 1234.5, "reset_daily": false}
  ```

#### MQTT Commands
Send commands to topic: `watermeter/command`
- `SET:1234.5` - Set counter to 1234.5 liters
- `RESET_DAILY` - Reset daily counter to 0
- `RESET_ALL` - Reset all counters to 0
- `STATUS` - Get current status

Responses published to: `watermeter/response`


### MQTT Topics

The system publishes data to the following MQTT topics:

- `watermeter/data` - Complete JSON data payload
- `watermeter/total` - Total consumption in liters
- `watermeter/daily` - Daily consumption in liters  
- `watermeter/status` - System status (ok/fault)
- `watermeter/backup` - Periodic data backup

### JSON Data Format

```json
{
  "total_liters": 12345,
  "daily_liters": 150,
  "pulse_count": 12345,
  "flow_rate": 2.5,
  "input_voltage": 24.1,
  "sensor_fault": false,
  "uptime": 3600000,
  "free_heap": 200000,
  "wifi_rssi": -45
}
```

### Home Assistant Integration

Add to your `configuration.yaml`:

```yaml
sensor:
  - platform: mqtt
    name: "Water Total Consumption"
    state_topic: "watermeter/total"
    unit_of_measurement: "L"
    icon: mdi:water

  - platform: mqtt
    name: "Water Daily Consumption"
    state_topic: "watermeter/daily"
    unit_of_measurement: "L"
    icon: mdi:water-pump

  - platform: mqtt
    name: "Water Flow Rate"
    state_topic: "watermeter/data"
    value_template: "{{ value_json.flow_rate }}"
    unit_of_measurement: "L/min"
    icon: mdi:water-pump
```

## Configuration

### Pin Configuration

| Function | GPIO Pin | Notes |
|----------|----------|-------|
| Pulse Input | GPIO2 | Interrupt capable, with protection circuit |
| Status LED | GPIO25 | External LED (avoids conflict with DomoticsCore) |
| Voltage Monitor | GPIO34 | ADC input for voltage monitoring |

### Timing Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| Save Interval | 30s | Data save to EEPROM frequency |
| Publish Interval | 60s | MQTT publish frequency |
| Backup Interval | 5min | Backup data frequency |
| Debounce Time | 50ms | Pulse debounce duration |

## Troubleshooting

### No Pulse Detection
1. Check 24V supply voltage at water meter
2. Verify protection circuit wiring
3. Test pulse signal with multimeter
4. Check GPIO pin configuration
5. Monitor serial output for debug messages

### False Pulses
1. Increase debounce time in configuration
2. Add capacitive filtering to input circuit
3. Check for electromagnetic interference
4. Verify proper grounding

### WiFi Connection Issues
1. Check WiFi credentials in DomoticsCore configuration
2. Verify WiFi signal strength
3. Check router settings and firewall
4. Monitor serial output for connection status

### MQTT Publishing Issues
1. Verify MQTT broker address and credentials
2. Check network connectivity
3. Test MQTT broker with external client
4. Review MQTT topic permissions

## Development

### Building from Source

```bash
# Install PlatformIO
pip install platformio

# Clone repository
git clone https://github.com/JN0V/WaterMeter.git
cd WaterMeter

# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Debug Mode

Enable debug output by setting in `platformio.ini`:

```ini
build_flags = 
    -DWATER_METER_DEBUG=1
    -DCORE_DEBUG_LEVEL=3
```

### Testing

Test the pulse detection without water flow:

```cpp
// Add to setup() for testing
void testPulseGeneration() {
    for(int i = 0; i < 10; i++) {
        pulseISR(); // Simulate pulse
        delay(1000);
    }
}
```

## Safety Warnings

⚠️ **ELECTRICAL SAFETY**
- Never connect 24V directly to ESP32 pins
- Always use proper circuit protection
- Verify voltage levels with multimeter before connection
- Use appropriate wire gauge for 24V connections

⚠️ **WATER SAFETY**
- Ensure waterproof enclosure for electronics
- Keep all electrical components away from water
- Use proper IP-rated enclosures for outdoor installation

## License

This project is open source. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the circuit protection documentation

## Version History

- **v0.2.0** - Added counter management interfaces (Web UI, REST API, MQTT commands, Serial)
- **v0.1.0** - Initial release with basic water meter monitoring
  - MQTT integration
  - Data persistence
  - Circuit protection documentation

## Related Projects

- [DomoticsCore](https://github.com/JN0V/DomoticsCore.git) - Core home automation library
- Water meter pulse sensors and magnetic switches
- Home Assistant integration examples
