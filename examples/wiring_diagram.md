# Wiring Diagrams and Examples

## Basic Breadboard Setup

### Method 1: Voltage Divider Protection Circuit

```
Water Meter                    Breadboard Layout                    ESP32
┌─────────────┐               ┌─────────────────────────────┐      ┌──────────┐
│             │               │  +24V Rail                  │      │          │
│   24V Pulse │───────────────│    │                        │      │          │
│   Output    │               │    │                        │      │          │
│             │               │   [22kΩ]                    │      │          │
│             │               │    │                        │      │          │
│    GND      │───────────────│    ├─────────[3.3kΩ]───GND │──────│ GPIO2    │
└─────────────┘               │    │           │            │      │          │
                              │    │      [Zener 3.6V]     │      │          │
                              │    │           │            │      │          │
                              │    └───────────┼────────────│──────│   GND    │
                              │                │            │      │          │
                              │               GND           │      │ GPIO25───│──[220Ω]──[LED]──GND
                              └─────────────────────────────┘      └──────────┘
```

### Method 2: Optocoupler Isolation

```
Water Meter                    Breadboard Layout                    ESP32
┌─────────────┐               ┌─────────────────────────────┐      ┌──────────┐
│             │               │  +24V Rail                  │      │          │
│   24V Pulse │───────────────│    │                        │      │          │
│   Output    │               │   [1kΩ]                     │      │          │
│             │               │    │                        │      │          │
│             │               │  ┌─┴─┐  PC817               │      │          │
│    GND      │───────────────│  │ 1 │ 2                    │      │          │
└─────────────┘               │  │   │                      │      │          │
                              │  │ 4 │ 3                    │      │          │
                              │  └─┬─┘                      │      │          │
                              │    │                        │      │          │
                              │   GND                       │      │          │
                              │                             │      │          │
                              │    3.3V──[10kΩ]──┬─────────│──────│ GPIO2    │
                              │                   │         │      │          │
                              │              Pin 3 (Collector)    │          │
                              │                   │         │      │          │
                              │              Pin 4 (Emitter)─────│   GND    │
                              │                             │      │ GPIO25───│──[220Ω]──[LED]──GND
                              └─────────────────────────────┘      └──────────┘
```

## Component Placement on Breadboard

### Voltage Divider Method

```
Breadboard Top View:
     a  b  c  d  e     f  g  h  i  j
   ┌─────────────────────────────────┐
 1 │ +  +  +  +  +     +  +  +  +  + │ +24V Rail
 2 │                                 │
 3 │    [22kΩ Resistor]              │
 4 │ 24V────┬─────────────────────── │ To GPIO2
 5 │        │                       │
 6 │    [3.3kΩ Resistor]            │
 7 │        │                       │
 8 │        ├─── [Zener Diode] ──── │
 9 │        │                       │
10 │ -  -  -┴- -     -  -  -  -  - │ GND Rail
11 │                                 │
12 │              [220Ω LED Resistor]│
13 │ GPIO25────────┬─────────────────│
14 │               │                 │
15 │              [LED]              │
16 │               │                 │
17 │ -  -  -  -  -┴-     -  -  -  - │ GND Rail
   └─────────────────────────────────┘

Components:
- 22kΩ resistor: Row 3, columns b-d
- 3.3kΩ resistor: Row 6, columns c-e  
- Zener diode: Row 8, columns c-f (cathode to GPIO side)
- 220Ω resistor: Row 12, columns f-h (for LED current limiting)
- LED: Row 15, columns g-h (anode to resistor, cathode to GND)
- Jumper wires: Connect junction point to ESP32 GPIO2, GPIO25 to LED circuit
```

### Optocoupler Method

```
Breadboard Top View:
     a  b  c  d  e     f  g  h  i  j
   ┌─────────────────────────────────┐
 1 │ +  +  +  +  +     +  +  +  +  + │ +24V Rail
 2 │                                 │
 3 │    [1kΩ Resistor]               │
 4 │ 24V────┬─────────────────────── │
 5 │        │                       │
 6 │     ┌──┴──┐ PC817               │
 7 │     │ 1 2 │                    │
 8 │     │ 4 3 │                    │
 9 │     └──┬──┘                    │
10 │        │                       │
11 │ -  -  -┴- -     -  -  -  -  - │ GND Rail (24V side)
12 │                                 │
13 │              3.3V──[10kΩ]──┬── │ To GPIO2
14 │                            │   │
15 │                         Pin 3  │
16 │                            │   │
17 │                         Pin 4──│ To ESP32 GND
18 │ -  -  -  - -     -  -  -  -┴- │ GND Rail (3.3V side)
19 │                                 │
20 │              [220Ω LED Resistor]│
21 │ GPIO25────────┬─────────────────│
22 │               │                 │
23 │              [LED]              │
24 │               │                 │
25 │ -  -  -  -  -┴-     -  -  -  - │ GND Rail
   └─────────────────────────────────┘

Components:
- 1kΩ resistor: Row 3, columns b-d
- PC817 optocoupler: Rows 6-9, columns c-d
- 10kΩ pull-up resistor: Row 13, columns g-i
- 220Ω resistor: Row 20, columns f-h (for LED current limiting)
- LED: Row 23, columns g-h (anode to resistor, cathode to GND)
```

## Pin Connections Summary

### ESP32 Connections
| ESP32 Pin | Function | Connection |
|-----------|----------|------------|
| GPIO2 | Pulse Input | Protection circuit output |
| GPIO25 | Status LED | 220Ω resistor → LED → GND |
| 3.3V | Power | Breadboard positive rail (3.3V side) |
| GND | Ground | Breadboard negative rail |
| EN | Reset | Optional: Pull-up resistor for stability |

### Water Meter Connections
| Water Meter | Function | Connection |
|-------------|----------|------------|
| Pulse + | 24V Signal | Protection circuit input |
| Pulse - | Ground | 24V ground rail |

## Testing Setup

### Initial Testing Without Water Meter

```cpp
// Test pulse detection with manual button
#define TEST_BUTTON_PIN 0  // Use BOOT button on ESP32

void setup() {
    // ... existing setup code ...
    
    // Add test button
    pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(TEST_BUTTON_PIN), testPulseISR, FALLING);
}

void IRAM_ATTR testPulseISR() {
    static unsigned long lastTestPulse = 0;
    if (millis() - lastTestPulse > 200) {  // 200ms debounce
        pulseCount++;
        Serial.println("Test pulse detected!");
        lastTestPulse = millis();
    }
}
```

### Voltage Testing Points

```
Test Points for Multimeter:
1. 24V Input: Should read 24V ±2V
2. After R1 (22kΩ): Should read ~3.1V during pulse
3. GPIO Pin: Should read 0V (no pulse) to 3.1V (pulse)
4. Zener Cathode: Should never exceed 3.6V
```

## Safety Checklist

- [ ] Double-check all connections before applying power
- [ ] Verify 24V supply polarity
- [ ] Confirm protection circuit values
- [ ] Test with multimeter before connecting ESP32
- [ ] Ensure proper grounding
- [ ] Use appropriate wire gauge (22 AWG minimum for 24V)
- [ ] Secure all connections to prevent shorts
- [ ] Keep water away from electronics

## Common Wiring Mistakes

1. **Reversed Zener Diode**: Cathode must face GPIO pin
2. **Wrong Resistor Values**: Double-check color codes
3. **Missing Ground Connection**: Both 24V and 3.3V sides need common ground
4. **Loose Connections**: Ensure solid breadboard connections
5. **Wrong GPIO Pin**: Use interrupt-capable pins (GPIO2, GPIO4, etc.)

## Troubleshooting Wiring Issues

### No Signal Detection
1. Check continuity from water meter to ESP32
2. Verify 24V supply with multimeter
3. Test protection circuit output voltage
4. Confirm GPIO pin configuration in code

### Erratic Readings
1. Add capacitive filtering (100nF across R2)
2. Check for loose connections
3. Verify proper grounding
4. Increase debounce time in software

### Voltage Too High
1. Recheck resistor values and connections
2. Verify Zener diode orientation
3. Measure actual 24V supply voltage
4. Check for short circuits in protection circuit
