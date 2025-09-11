# Water Meter Sensor Connection

## Overview

This document describes how to connect a water meter sensor with ESP32-compatible voltage levels (3.5V/0.2V) directly to an ESP32 microcontroller.

## Circuit Requirements

### Input Signal Characteristics
- **Voltage**: 3.5V (active) / 0.2V (inactive) - ESP32 compatible levels
- **Pulse Duration**: Typically 50-200ms per liter
- **Pulse Frequency**: Variable based on water flow rate
- **Signal Type**: Hall effect sensor with built-in voltage regulation

### Connection Method

#### Direct Connection (No Protection Needed)

```
Sensor Signal ----+---- ESP32 GPIO Pin
                  |
                  +----[R1: 220Ω]----[LED]---- GND (optional status indicator)

Sensor GND -------+---- ESP32 GND
```

**Component Values:**
- R1: 220Ω (1/4W, 5% tolerance) - LED current limiting resistor (optional)
- LED: Standard 5mm LED (red/green/blue, any color) - optional status indicator
- Optional: 100nF ceramic capacitor from GPIO to GND for noise filtering

**Signal Levels:**
- High (pulse active): 3.5V - Safe for ESP32 (below 3.6V absolute maximum)
- Low (no pulse): 0.2V - Clean logic low for ESP32

#### Alternative: Optocoupler Isolation (For Noisy Environments)

```
Sensor Signal ----[R2: 1kΩ]----[LED]----[Optocoupler]----[R3: 10kΩ]---- 3.3V
                                              |                    |
Sensor GND -------------------------------+  GND              ESP32 GPIO
```

**Component Values:**
- R2: 1kΩ current limiting resistor
- R3: 10kΩ pull-up resistor  
- Optocoupler: PC817, 4N35, or similar
- Note: Only needed in electrically noisy industrial environments

### Breadboard Implementation

#### Method 1: Direct Connection (Recommended)

1. **Power Rails Setup:**
   - Connect ESP32 3.3V to breadboard positive rail
   - Connect ESP32 GND to breadboard negative rail

2. **Sensor Connection:**
   - Connect sensor signal wire directly to ESP32 GPIO2 (interrupt capable)
   - Connect sensor GND wire to ESP32 GND

3. **Status LED (Optional Visual Pulse Indicator):**
   - Connect R1 (220Ω) from GPIO2 to LED anode
   - Connect LED cathode to GND
   - LED will light up when pulse is active

4. **Optional Filtering:**
   - Add 100nF capacitor from GPIO2 to GND for noise filtering

#### Method 2: Optocoupler (For Noisy Environments)

1. **Input Side (Sensor):**
   - Connect sensor signal through 1kΩ resistor to optocoupler LED anode
   - Connect optocoupler LED cathode to sensor GND

2. **Output Side (ESP32):**
   - Connect optocoupler collector to ESP32 3.3V through 10kΩ pull-up
   - Connect optocoupler emitter to ESP32 GND
   - Connect junction (collector) to ESP32 GPIO2

## Safety Considerations

### Electrical Safety
- Sensor outputs 3.5V max - safe for ESP32 (below 3.6V absolute maximum)
- Ensure all connections are secure to prevent intermittent faults
- Use heat shrink tubing or electrical tape on exposed connections

### Component Ratings
- LED current limiting resistor: 1/4W sufficient for 220Ω
- All components operate at low voltage (≤3.5V)

### Testing Procedure

1. **Voltage Verification:**
   ```
   Multimeter Test Points:
   - Sensor signal: Should read 0.2V (no pulse) to 3.5V (pulse active)
   - GPIO input: Same as sensor signal (direct connection)
   - Verify signal is within ESP32 safe range (0-3.6V)
   ```

2. **Pulse Detection Test:**
   - Use oscilloscope or logic analyzer to verify pulse shape
   - Check for proper rise/fall times
   - Verify debounce timing is adequate

3. **Isolation Test (for optocoupler method):**
   - Verify no electrical continuity between 24V and 3.3V sides
   - Test with megohm meter for proper isolation

## Troubleshooting

### Common Issues

1. **No Pulse Detection:**
   - Check 24V supply voltage
   - Verify wiring connections
   - Test with multimeter in pulse mode
   - Check GPIO pin configuration in software

2. **False Pulses:**
   - Add or increase debounce time in software
   - Add capacitive filtering (100nF-1µF)
   - Check for electromagnetic interference
   - Verify proper grounding

3. **Voltage Too High/Low:**
   - Recalculate voltage divider values
   - Check component tolerances
   - Verify resistor values are correct

### Component Substitutions

| Original | Alternative | Notes |
|----------|-------------|-------|
| 200kΩ | 100kΩ + 100kΩ in series | Use two 100kΩ resistors |
| 200kΩ | 1MΩ + adjust R2 to 5kΩ | Maintains ~20:1 ratio |
| 10kΩ | 5kΩ + 5kΩ in series | Use two 5kΩ resistors |
| PC817 | 4N35, 6N137 | Verify pinout differences |

## Schematic Symbols

```
Resistor:     ----[R]----
Optocoupler:  [LED>|<Transistor]
Capacitor:    ----||----
```

## Bill of Materials

### Method 1 (Direct Connection - Recommended)
- 1× 220Ω resistor, 1/4W, 5% (for optional status LED)
- 1× LED (3mm or 5mm, any color) - optional
- 1× 100nF ceramic capacitor (optional noise filtering)
- Breadboard jumper wires

### Method 2 (Optocoupler - For Noisy Environments)
- 1× PC817 optocoupler
- 1× 1kΩ resistor, 1/4W, 5%
- 1× 10kΩ resistor, 1/4W, 5%
- 1× 220Ω resistor, 1/4W, 5% (for status LED)
- 1× LED (3mm or 5mm, any color) - optional
- Breadboard jumper wires

**Total Cost:** ~$1-4 USD depending on method chosen
