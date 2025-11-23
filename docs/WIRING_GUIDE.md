# Water Meter Wiring Guide - ESP32 Isolation Solution

## Problem Description

The ESP32 shares the magnetic sensor signal with the main electronic control box (which also controls CO2 injection). When the ESP32 is powered off or reboots, it creates electrical disturbances that trigger false pulses on the main system, incorrectly counting 1 liter and activating the CO2 bottle.

## Sensor Behavior

- **Magnetic sensor output:**
  - No magnet (normal): **3.5V** (HIGH)
  - Magnet under sensor: **0.2V** (LOW)
  
- **Water consumption:**
  - 1 complete rotation = 1 liter
  - Magnet stays under sensor for a fraction of the rotation
  
- **Correct pulse detection:**
  - Count 1 liter when magnet **LEAVES** the sensor (LOW → HIGH transition)
  - This is the **RISING edge** (0.2V → 3.5V)

## Solution: Electrical Isolation with High-Impedance Buffer

### Required Components

- **1x MOSFET IRLZ24N, IRLZ34N, or IRLZ44N** (N-channel logic-level MOSFET)
- **1x resistor 10kΩ** (pull-down for MOSFET gate)
- **1x resistor 10kΩ** (pull-up for ESP32 input)
- **1x capacitor 0.1 μF (100nF)** - Use your 50V 0.1 μF capacitor for signal filtering
- **Jumper wires** - Suggested colors:
  - **RED:** 3.3V power
  - **BLACK:** GND (ground)
  - **YELLOW:** Sensor signal
  - **BLUE:** GPIO34 connection
  - **GREEN:** MOSFET connections

### Circuit Diagram (MOSFET IRLZ24N/34N/44N)

**⚠️ IMPORTANT - COMMON GROUND (GND):**
- **All GND are connected together** (ESP32 GND, MOSFET SOURCE, sensor GND)
- This is the **same reference potential** (0V) for the entire circuit
- Use **black wires** for all GND connections

```
Water Meter Sensor (3.5V/0.2V output)
         │
         ├──────────────────────────────► Main Control Box (unchanged)
         │
         └───[ YELLOW ]───┬─────────────► MOSFET GATE (Pin G)
                          │
                     [ 10kΩ ]───[ BLACK ]─► COMMON GND
                     (pull-down)

                           
ESP32 3.3V ──[ RED ]──[ 10kΩ ]──┬──[ BLUE ]──► GPIO34 (ESP32)
                     (pull-up)  │
                                │
                          [ GREEN ]──► MOSFET DRAIN (Pin D)
                                │
                          [ 0.1μF ]──[ BLACK ]─► COMMON GND
                          (ceramic)
                           
ESP32 GND ──[ BLACK ]───────────────────────────► MOSFET SOURCE (Pin S)
                                                  │
                                                  └──► COMMON GND
                                                  │
Sensor GND ──[ BLACK ]────────────────────────────┘


MOSFET Pinout (IRLZ24N/34N/44N - TO-220 package):
┌─────────────────────┐
│     Front View      │
│   (metal tab back)  │
│                     │
│   G   D   S         │  G=Gate, D=Drain, S=Source
│   │   │   │         │
└───┼───┼───┼─────────┘
    │   │   │
  GATE DRAIN SOURCE
```

### Suggested Wire Colors

| Color | Usage | Examples |
|-------|-------|----------|
| **BLACK** | GND (common ground) | All GND (ESP32, MOSFET SOURCE, sensor, capacitor) |
| **RED** | Positive power | ESP32 3.3V |
| **YELLOW** | Sensor signal | Sensor output → MOSFET GATE |
| **BLUE** | ESP32 GPIO | Pull-up resistor → GPIO34 |
| **GREEN** | MOSFET DRAIN | MOSFET DRAIN → GPIO34 |
| **WHITE** | Optional | Secondary connections if needed |

### Detailed Connections - Step by Step

**⚠️ FUNDAMENTAL RULE - COMMON GND:**
All GND (ESP32, MOSFET SOURCE, sensor) are **connected together** to the GND rail of your breadboard. This is the **same electrical point** (0V reference potential).

#### 1. Establish Common GND (BLACK)

- **ESP32 GND** → Breadboard GND rail (BLACK wire)
- **Sensor GND** → Breadboard GND rail (BLACK wire)
- **MOSFET SOURCE (Pin S)** → Breadboard GND rail (BLACK wire)

*All these GND are now at the same potential.*

#### 2. Sensor → Main Control Box (unchanged)

- **Sensor signal** → Main control box input (existing, do not modify)

#### 3. Sensor → MOSFET GATE (YELLOW + pull-down resistor)

- **Sensor signal** → **MOSFET GATE (Pin G)** (YELLOW wire)
- **10kΩ resistor** between **MOSFET GATE (Pin G)** and **COMMON GND** (BLACK rail)
  
  *This resistor "pulls" GATE towards 0V when sensor is at 0.2V*

#### 4. MOSFET → ESP32 (GPIO34)

- **ESP32 3.3V** → **10kΩ resistor** → **GPIO34** (RED then BLUE wire)
  
  *This resistor "pulls" GPIO34 towards 3.3V when MOSFET is OFF*

- **MOSFET DRAIN (Pin D)** → **GPIO34** (GREEN or BLUE wire)
  
  *DRAIN connects to the same point as pull-up resistor*

#### 5. Filtering (0.1 μF capacitor)

- **One capacitor leg** → **GPIO34** (same point as DRAIN and resistor)
- **Other capacitor leg** → **COMMON GND** (BLACK rail)
  
  *Capacitor filters electrical noise on GPIO34*

**⚠️ CAPACITOR POLARITY:**
- **0.1 μF (100nF) ceramic capacitor is NON-POLARIZED**
- **No polarity to respect** - you can insert it in any direction
- Both legs are the same length (or similar)
- If your capacitor has different leg lengths, it's likely electrolytic (wrong type)
- Ceramic capacitors are typically small, round or rectangular, beige/brown color

#### GND Summary (all at same location)

```
Breadboard GND rail (BLACK)
    ├── ESP32 GND
    ├── MOSFET SOURCE (Pin S)
    ├── Sensor GND
    ├── 10kΩ resistor pull-down (from GATE)
    └── 0.1 μF capacitor (from GPIO34)
```

**All these GND are the same electrical point = 0V reference**

### How It Works

1. **When sensor is HIGH (3.5V - no magnet):**
   - MOSFET GATE at 3.5V → MOSFET ON (logic-level compatible)
   - DRAIN pulls GPIO34 to GND → **ESP32 reads LOW**

2. **When sensor is LOW (0.2V - magnet present):**
   - MOSFET GATE at 0.2V → MOSFET OFF (below threshold)
   - 10kΩ pull-up pulls GPIO34 to 3.3V → **ESP32 reads HIGH**

3. **Isolation benefits:**
   - **Ultra-high impedance on sensor line** (~10¹² Ω gate input)
   - **When ESP32 is OFF:** MOSFET gate floating with pull-down, zero current
   - **No back-feeding** - Perfect isolation between sensor and ESP32
   - **Main box unaffected** by ESP32 power state
   - **Lower voltage drop** than bipolar transistor (RDS(on) < 0.04Ω)

### Signal Inversion

⚠️ **IMPORTANT:** The MOSFET inverts the signal:
- Sensor HIGH (3.5V) → ESP32 reads LOW
- Sensor LOW (0.2V) → ESP32 reads HIGH

This is **PERFECT** for detecting liter completion:
- **Magnet arrives** (sensor 3.5V→0.2V) → ESP32 sees LOW→HIGH (ignored)
- **Magnet leaves** (sensor 0.2V→3.5V) → ESP32 sees HIGH→LOW = **FALLING edge** ✓

**Why IRLZ series is ideal:**
- Logic-level threshold: VGS(th) = 1-2V (works with 3.5V sensor)
- Very low RDS(on): < 0.04Ω (minimal voltage drop)
- High current capability: 20-50A (overkill but excellent for reliability)
- No base current needed (vs bipolar transistor)

## Breadboard Wiring Steps

### Preparation
1. **Disconnect ESP32** from all power sources
2. **Locate sensor output** at the junction box

### Step-by-Step Assembly

**Step 1 - Establish Common GND (BLACK)**
```
a. ESP32 GND (GND pin) → Breadboard GND rail (BLACK wire)
b. Insert MOSFET on breadboard (identify G-D-S with datasheet)
c. MOSFET SOURCE (Pin S) → Breadboard GND rail (BLACK wire)
d. Prepare connection for Sensor GND → GND rail (BLACK wire)
```

**Step 2 - Sensor → MOSFET GATE (YELLOW + pull-down)**
```
a. Sensor signal → MOSFET GATE (Pin G) (YELLOW wire)
b. 10kΩ resistor: one leg on GATE (Pin G), other leg on GND rail (BLACK)
   └─ This resistor "pulls" GATE towards GND when sensor = 0.2V
```

**Step 3 - Pull-up GPIO34 (RED + BLUE)**
```
a. ESP32 3.3V (3V3 pin) → 10kΩ resistor (RED wire)
b. 10kΩ resistor → GPIO34 (pin 34) (BLUE wire)
   └─ This resistor "pulls" GPIO34 towards 3.3V when MOSFET = OFF
```

**Step 4 - MOSFET DRAIN Connection (GREEN)**
```
a. MOSFET DRAIN (Pin D) → GPIO34 (GREEN or BLUE wire)
   └─ Connects to SAME point as pull-up resistor
```

**Step 5 - Filtering (0.1 μF capacitor)**
```
a. 0.1 μF (50V) capacitor: one leg on GPIO34
b. Other capacitor leg → GND rail (BLACK)
   └─ As close as possible to ESP32 to filter noise
   
⚠️ CAPACITOR: 0.1 μF ceramic is NON-POLARIZED - no orientation needed
```

### Multimeter Verification (BEFORE powering on)

**Continuity test:**
1. Verify all GND are connected (ESP32 GND, MOSFET SOURCE, GND rail)
2. Verify NO short circuit between 3.3V and GND
3. Verify NO short circuit between GPIO34 and GND (should show ~10kΩ)

**Voltage test (after powering ESP32):**
1. **Without sensor signal (or sensor at 3.5V):** GPIO34 should be ~0V
2. **With sensor at 0.2V (or disconnected):** GPIO34 should be ~3.3V

### Power On
1. **Connect ESP32** (USB or power supply)
2. **Check serial logs** (see next section)

## Notes on Component Selection

**IRLZ24N vs IRLZ34N vs IRLZ44N:**
- All are logic-level N-channel MOSFETs (perfect for 3.3V/5V logic)
- Differences: current rating (24N=17A, 34N=27A, 44N=47A)
- Any of these works perfectly for this application
- Choose based on availability

**Why not LM317T or L7805?**
- These are voltage regulators, not switches
- Not suitable for this application

## Safety Checklist

- [ ] MOSFET correctly oriented (G-D-S pinout - check datasheet)
- [ ] No short circuits between pins
- [ ] Pull-up resistor connected to GPIO34 (10kΩ to 3.3V)
- [ ] Pull-down resistor connected to GATE (10kΩ to GND)
- [ ] ESP32 GND connected to MOSFET SOURCE
- [ ] Sensor connected directly to GATE (no resistor needed)
- [ ] Test with multimeter before powering ESP32

## Expected Behavior

✓ **Main box operates normally** even when ESP32 is off
✓ **No false pulses** on main box when ESP32 boots/reboots
✓ **ESP32 counts accurately** when magnet completes rotation (leaves sensor)
✓ **No false counts on ESP32 boot** (handled in software with initialization delay)

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| No pulses detected | Wrong pinout | Check G-D-S orientation on MOSFET |
| Continuous pulses | Short circuit | Check DRAIN/SOURCE not swapped |
| Main box affected | Missing pull-down | Verify 10kΩ gate pull-down to GND |
| Erratic readings | No filtering | Add 100nF capacitor on GPIO34 |
| GPIO34 always LOW | No pull-up | Check 10kΩ resistor to 3.3V |

---

**Next step:** Update software to handle FALLING edge with boot protection
