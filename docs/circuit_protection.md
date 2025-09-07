# Circuit Protection for 24V Water Meter Pulse Input

## Overview

This document describes the circuit protection required to safely interface a 24V pulse signal from a water meter to an ESP32 microcontroller operating at 3.3V logic levels.

## Circuit Requirements

### Input Signal Characteristics
- **Voltage**: 24V DC pulse signal
- **Pulse Duration**: Typically 50-200ms per liter
- **Pulse Frequency**: Variable based on water flow rate
- **Signal Type**: Magnetic reed switch or hall effect sensor

### Protection Circuit Components

#### 1. Voltage Divider with Zener Protection

```
24V Pulse ----[R1: 22kΩ]----+----[R2: 3.3kΩ]---- GND
                             |
                             +---- ESP32 GPIO Pin
                             |
                        [Zener 3.6V]
                             |
                            GND
```

**Component Values:**
- R1: 22kΩ (1/4W, 5% tolerance)
- R2: 3.3kΩ (1/4W, 5% tolerance)  
- Zener Diode: 3.6V, 500mW (BZX79C3V6 or equivalent)
- Optional: 100nF ceramic capacitor parallel to R2 for noise filtering

**Voltage Calculation:**
- Vout = Vin × (R2 / (R1 + R2))
- Vout = 24V × (3.3kΩ / (22kΩ + 3.3kΩ)) = 3.13V

#### 2. Optocoupler Isolation (Recommended for Industrial Applications)

```
24V Pulse ----[R3: 1kΩ]----[LED]----[Optocoupler]----[R4: 10kΩ]---- 3.3V
                                          |                    |
                                         GND              ESP32 GPIO
```

**Component Values:**
- R3: 1kΩ current limiting resistor
- R4: 10kΩ pull-up resistor
- Optocoupler: PC817, 4N35, or similar
- Recommended: PC817C for better CTR (Current Transfer Ratio)

### Breadboard Implementation

#### Method 1: Voltage Divider (Simple)

1. **Power Rails Setup:**
   - Connect ESP32 3.3V to breadboard positive rail
   - Connect ESP32 GND to breadboard negative rail

2. **Voltage Divider Assembly:**
   - Place R1 (22kΩ) between 24V input and junction point
   - Place R2 (3.3kΩ) between junction point and GND
   - Connect junction point to ESP32 GPIO2 (interrupt capable)

3. **Protection:**
   - Connect Zener diode (3.6V) from GPIO line to GND (cathode to GPIO)
   - Add 100nF capacitor parallel to R2 for noise filtering

#### Method 2: Optocoupler (Isolated)

1. **Input Side (24V):**
   - Connect 24V pulse through 1kΩ resistor to optocoupler LED anode
   - Connect optocoupler LED cathode to 24V GND

2. **Output Side (3.3V):**
   - Connect optocoupler collector to ESP32 3.3V through 10kΩ pull-up
   - Connect optocoupler emitter to ESP32 GND
   - Connect junction (collector) to ESP32 GPIO2

## Safety Considerations

### Electrical Safety
- **Never connect 24V directly to ESP32 pins** - this will damage the microcontroller
- Use proper wire gauge for 24V connections (minimum 22 AWG)
- Ensure all connections are secure to prevent intermittent faults
- Use heat shrink tubing or electrical tape on exposed connections

### Component Ratings
- Ensure all resistors are rated for at least 1/4W power dissipation
- Zener diode should handle at least 500mW
- Use components with appropriate voltage ratings (>30V for 24V side)

### Testing Procedure

1. **Voltage Verification:**
   ```
   Multimeter Test Points:
   - 24V input: Should read 24V ±2V
   - GPIO input: Should read 0V (no pulse) to 3.1-3.3V (pulse active)
   - Never exceed 3.6V on GPIO pin
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
   - Verify Zener diode is functioning

### Component Substitutions

| Original | Alternative | Notes |
|----------|-------------|-------|
| 22kΩ | 20kΩ-27kΩ | Adjust R2 accordingly |
| 3.3kΩ | 3.0kΩ-3.6kΩ | Maintain ~8:1 ratio |
| PC817 | 4N35, 6N137 | Verify pinout differences |
| BZX79C3V6 | 1N4728A | 3.3V Zener alternative |

## Schematic Symbols

```
Resistor:     ----[R]----
Zener:        ----[Z]----  (with arrow pointing to cathode)
Optocoupler:  [LED>|<Transistor]
Capacitor:    ----||----
```

## Bill of Materials

### Method 1 (Voltage Divider)
- 1× 22kΩ resistor, 1/4W, 5%
- 1× 3.3kΩ resistor, 1/4W, 5%
- 1× 3.6V Zener diode, 500mW
- 1× 100nF ceramic capacitor (optional)
- 1× 220Ω resistor, 1/4W, 5% (for status LED)
- 1× LED (3mm or 5mm, any color)
- Breadboard jumper wires

### Method 2 (Optocoupler)
- 1× PC817 optocoupler
- 1× 1kΩ resistor, 1/4W, 5%
- 1× 10kΩ resistor, 1/4W, 5%
- 1× 220Ω resistor, 1/4W, 5% (for status LED)
- 1× LED (3mm or 5mm, any color)
- Breadboard jumper wires

**Total Cost:** ~$3-6 USD depending on method chosen
