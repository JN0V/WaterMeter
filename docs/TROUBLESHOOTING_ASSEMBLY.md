# Assembly Troubleshooting Guide

## Short Circuit Detection (Your Problem!)

### Your Measurements:
- **GPIO34 to GND: 5Ω** (should be ~10kΩ)
- **3.3V to GND: 1.5Ω** (should be >10kΩ or infinite)

**⚠️ CRITICAL: DO NOT POWER ON ESP32 - YOU HAVE A SHORT CIRCUIT!**

## What These Measurements Mean

### 1.5Ω between 3.3V and GND = **DIRECT SHORT CIRCUIT**
This means 3.3V is almost directly connected to GND, which will:
- Draw excessive current (potentially 2A+)
- Damage the ESP32 voltage regulator
- Overheat components
- Possibly damage USB port or power supply

### 5Ω between GPIO34 and GND = **MOSFET Conducting or Shorted**
This indicates:
- MOSFET DRAIN-SOURCE is conducting (MOSFET is ON when it should be OFF)
- OR GPIO34 is directly shorted to GND
- Pull-up resistor (10kΩ) is being bypassed by a low resistance path

## Root Cause Analysis

### Most Likely Causes (in order of probability):

#### 1. **MOSFET Pins Swapped** (90% probability)
If DRAIN and SOURCE are reversed, the circuit behavior is completely wrong.

**Correct pinout (front view, metal tab on back):**
```
   ┌─────┐
   │IRLZ │
   │24N  │
   └─┬─┬─┘
     │ │ │
     G D S
   GATE DRAIN SOURCE
```

**Check your MOSFET:**
- Left pin = GATE (receives sensor signal YELLOW + 10kΩ to GND)
- Middle pin = DRAIN (goes to GPIO34 GREEN)
- Right pin = SOURCE (goes to GND rail BLACK)

**If SOURCE and DRAIN are swapped:**
- The MOSFET will act as a short circuit
- This explains both your measurements!

#### 2. **Pull-down Resistor Misplaced** (60% probability)
The 10kΩ pull-down must be between GATE and GND, not between DRAIN and GND.

**Correct:**
```
Sensor Signal (YELLOW) ──┬── MOSFET GATE
                         │
                      [10kΩ]
                         │
                        GND
```

**Incorrect (causes short):**
```
Sensor Signal ── MOSFET GATE

MOSFET DRAIN ──┬── GPIO34
               │
            [10kΩ]  ← WRONG! This creates a parallel path
               │       to GND, lowering resistance
              GND
```

#### 3. **Breadboard Internal Short** (30% probability)
Sometimes breadboard holes are damaged or have debris causing unintended connections.

**Test:** Remove all components and check:
```
ESP32 unplugged:
- Measure 3.3V pin to GND pin on ESP32 itself → should be OPEN (infinite)
- If you get low resistance, your ESP32 may be damaged
```

#### 4. **Wrong Resistor Value** (20% probability)
If you used a very low value resistor (e.g., 10Ω instead of 10kΩ), this would cause your measurements.

**Verify resistor color bands:**
- 10kΩ = Brown-Black-Orange-Gold
- If you have Brown-Black-Black-Gold = 10Ω → WRONG!

## Step-by-Step Diagnosis

### Phase 1: Component Verification (ESP32 OFF, everything unplugged)

**Step 1: Check ESP32 itself**
```
1. Remove ESP32 from breadboard
2. Measure between 3.3V pin and GND pin on ESP32 → should be OPEN
3. If <1kΩ, your ESP32 may be damaged or have internal short
```

**Step 2: Check MOSFET pinout**
```
1. Remove MOSFET from breadboard
2. With datasheet, verify which pin is G, D, S
3. Use a marker to label the pins if needed
4. Measure DRAIN-SOURCE with multimeter:
   - Should be OPEN (infinite resistance) when GATE is floating
   - Should be ~0Ω when GATE is connected to +5V (MOSFET ON)
```

**Step 3: Verify resistor values**
```
1. Remove both 10kΩ resistors from breadboard
2. Measure each resistor → both should be 9-11kΩ
3. If one shows <100Ω, you grabbed the wrong resistor!
```

### Phase 2: Rebuild Step-by-Step with Checks

**Step 1: GND rail only**
```
1. Place ESP32 on breadboard (unpowered)
2. Connect ESP32 GND → GND rail (BLACK wire)
3. Measure ESP32 GND to 3.3V pin → should be OPEN
   ✓ If OPEN: continue
   ✗ If low resistance: STOP - breadboard or ESP32 problem
```

**Step 2: Add MOSFET (verify orientation!)**
```
1. Insert MOSFET with correct G-D-S orientation
2. Connect MOSFET SOURCE → GND rail (BLACK wire)
3. Measure MOSFET SOURCE to ESP32 GND → should be 0Ω (beep)
4. Measure MOSFET DRAIN to GND → should be OPEN (MOSFET OFF)
   ✓ If OPEN: continue
   ✗ If low resistance: MOSFET pins are swapped!
```

**Step 3: Add pull-down resistor (GATE to GND)**
```
1. Connect 10kΩ resistor: one leg to MOSFET GATE, other leg to GND rail
2. Measure MOSFET GATE to GND → should be ~10kΩ
   ✓ If 9-11kΩ: continue
   ✗ If <1kΩ: resistor is wrong value or shorted
```

**Step 4: Add pull-up resistor (3.3V to GPIO34)**
```
1. Connect 10kΩ resistor: one leg to ESP32 3.3V, other leg to GPIO34
2. DO NOT connect MOSFET DRAIN yet
3. Measure GPIO34 to GND → should be ~10kΩ
4. Measure 3.3V to GND → should be OPEN or >10kΩ
   ✓ If correct: continue
   ✗ If low resistance: STOP - you have a short somewhere
```

**Step 5: Connect MOSFET DRAIN to GPIO34**
```
1. Connect MOSFET DRAIN → GPIO34 (GREEN wire, same point as pull-up)
2. Measure GPIO34 to GND:
   - MOSFET OFF (GATE floating) → should be ~10kΩ (pull-up)
   - Touch GATE to 3.3V → should drop to near 0Ω (MOSFET ON)
   ✓ If behavior correct: continue
   ✗ If always low resistance: MOSFET pins swapped or bad MOSFET
```

**Step 6: Add capacitor**
```
1. Connect 0.1μF capacitor between GPIO34 and GND (no polarity)
2. Measure GPIO34 to GND → should still be ~10kΩ
3. Measure 3.3V to GND → should still be OPEN or >10kΩ
   ✓ If correct: safe to power on
```

## Expected Final Measurements (Before Power On)

With ESP32 **UNPOWERED**, MOSFET GATE **floating** (sensor disconnected):

| Measurement | Expected Value | Your Value | Status |
|-------------|----------------|------------|--------|
| 3.3V to GND | >10kΩ or OPEN | 1.5Ω | ❌ SHORT! |
| GPIO34 to GND | ~10kΩ (pull-up) | 5Ω | ❌ SHORT! |
| ESP32 GND to MOSFET SOURCE | 0Ω (beep) | ? | Check this |
| MOSFET GATE to GND | ~10kΩ (pull-down) | ? | Check this |

## Quick Visual Check

**Correct wiring summary:**
```
ESP32 3.3V ──[RED]── 10kΩ ──[BLUE]── GPIO34
                                       │
                                       ├── MOSFET DRAIN [GREEN]
                                       │
                                       └── Capacitor 0.1μF ──[BLACK]── GND
                                       
MOSFET GATE [YELLOW] ──┬── Sensor signal
                       │
                    10kΩ (pull-down)
                       │
                      GND

MOSFET SOURCE [BLACK] ── GND

ESP32 GND [BLACK] ── GND

Sensor GND [BLACK] ── GND

(All GND to same breadboard rail!)
```

## If Problem Persists

1. **Take a clear photo** of your breadboard from above
2. **Check each wire** against the corrected diagram
3. **Verify MOSFET orientation** with a magnifying glass if needed
4. **Try a different breadboard** (yours may have internal shorts)
5. **Try a different MOSFET** (yours may be defective)

## Safe to Power On When:

✅ 3.3V to GND: >10kΩ or OPEN  
✅ GPIO34 to GND: ~10kΩ (with GATE floating)  
✅ ESP32 GND to MOSFET SOURCE: 0Ω (continuity)  
✅ Visual inspection matches diagram  
✅ No visible shorts or crossed wires  

**Only then is it safe to connect USB power!**

---

**REMEMBER:** Your current measurements (1.5Ω and 5Ω) indicate a short circuit. **Do not power on** until these are resolved!
