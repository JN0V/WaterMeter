# Water Meter Testing Guide

## Pre-Installation Testing

**⚠️ Test on breadboard before connecting to water meter!**

### 1. Breadboard Test

Before connecting to your water meter, build and test the isolation circuit on a breadboard.

#### Materials needed:
- Breadboard
- MOSFET IRLZ24N/34N/44N (N-channel logic-level)
- 2x 10kΩ resistors (gate pull-down + GPIO pull-up)
- 100nF capacitor (optional but recommended)
- ESP32
- Variable power supply 0-5V (to simulate sensor)

#### Test Circuit:

```
Test Power Supply (0-5V) ──┬──────── MOSFET GATE (IRLZ24N)
                           │
                      [ 10kΩ ]────── GND (pull-down)
                                      
ESP32 GND ──────────────────────────── MOSFET SOURCE

ESP32 3.3V ──[ 10kΩ ]───┬──────────── GPIO34 (ESP32)
                        │
                   MOSFET DRAIN
                        │
                   [ 100nF ]──────── GND (near ESP32)
```

#### Test Procedure:

1. **Build circuit on breadboard**
2. **Flash firmware** to ESP32 with serial console enabled
3. **Power ESP32** and watch serial output:
   ```
   [WATER] Initial GPIO state: HIGH (magnet NOT under sensor)
   [WATER] ⏳ Pulse detection disabled for 3000 ms (boot protection)
   ```

4. **Test signal inversion** with power supply:
   | Input Voltage | Expected GPIO34 | Expected Serial Log |
   |--------------|-----------------|---------------------|
   | 3.5V (no magnet) | LOW (~0V) | "HIGH (magnet NOT under)" |
   | 0.2V (magnet) | HIGH (3.3V) | "LOW (magnet UNDER)" |

5. **Test pulse detection:**
   - Wait 3 seconds (boot protection)
   - Apply 0.2V → 3.5V transition (simulate magnet leaving)
   - **Expected:** Serial shows `[SENSOR] ✓ Pulse detection enabled`
   - Then: `[SENSOR] PULSE: count=1, daily=1L, yearly=1L`

6. **Test debounce:**
   - Rapid transitions < 500ms should be ignored
   - Only transitions > 500ms should count

#### Success Criteria:

- ✓ Signal correctly inverted (3.5V → GPIO LOW, 0.2V → GPIO HIGH)
- ✓ No pulses counted during first 3 seconds after boot
- ✓ Pulses counted after boot delay
- ✓ Debounce working (ignores rapid transitions)
- ✓ LED flashes on each valid pulse

---

## Installation Testing (Water Meter Connection)

**Only proceed if breadboard tests passed!**

### 1. Physical Installation

1. **Power OFF** both ESP32 and main control box
2. **Install transistor circuit** near ESP32 (use perfboard or PCB)
3. **Connect wires:**
   - Sensor signal (from junction box) → 1kΩ resistor → Transistor BASE
   - Transistor EMITTER → ESP32 GND
   - Transistor COLLECTOR → GPIO34
   - ESP32 3.3V → 10kΩ resistor → GPIO34
   - Add 100nF capacitor between GPIO34 and GND

4. **Verify connections** with multimeter (power still OFF)
5. **Double-check:** Sensor line has NO direct connection to ESP32

### 2. Power-On Sequence

**CRITICAL ORDER:**

1. **Power ON main control box** (let it stabilize 10 seconds)
2. **Verify main box works** normally (check display, CO2 control, etc.)
3. **Power ON ESP32** (connect USB or power supply)
4. **Watch serial console:**
   ```
   [WATER] Initializing water meter...
   [WATER] GPIO: PULSE=34 (with transistor buffer), LED=32
   [WATER] Initial GPIO state: LOW (magnet UNDER sensor)  ← or HIGH if no magnet
   [WATER] Interrupt attached to GPIO 34 (FALLING edge)
   [WATER] ⏳ Pulse detection disabled for 3000 ms (boot protection)
   [WATER] Water meter ready: 0 pulses (0.000 m³)
   ```

5. **Wait 3 seconds** - verify no false pulses counted
6. **Expected after 3s:** `[SENSOR] ✓ Pulse detection enabled after 3000 ms`

### 3. Functional Testing

#### Test 1: Normal Water Consumption

1. **Open faucet** and let 1 liter flow
2. **Watch physical meter:** Rotating indicator should complete 1 full rotation
3. **Check serial console:**
   ```
   [SENSOR] PULSE: count=1, daily=1L, yearly=1L
   ```
4. **Verify LED** on GPIO32 flashes briefly
5. **Repeat** for 5-10 liters to verify consistency

#### Test 2: Main Control Box Independence

1. **Power OFF ESP32** while main box is ON
2. **Check main box:** Should NOT register any false pulses
3. **Check CO2 bottle:** Should NOT activate
4. **Power ON ESP32**
5. **Check main box:** Still no false pulses
6. **✓ SUCCESS:** Main box unaffected by ESP32 power state

#### Test 3: Boot with Magnet Under Sensor

1. **Open faucet** slightly to position magnet under sensor
2. **Stop water** when `Initial GPIO state: LOW (magnet UNDER sensor)` in logs
3. **Reboot ESP32:** `ESP.restart()` via console or power cycle
4. **Watch serial:** Should show `Initial GPIO state: LOW (magnet UNDER sensor)`
5. **Wait 3 seconds** (boot protection)
6. **Expected:** No false pulse counted
7. **Open faucet again** to complete the liter
8. **Expected:** Pulse counted when magnet LEAVES sensor

#### Test 4: WebUI and Home Assistant

1. **Connect to WiFi** via WebUI (http://192.168.4.1 or http://watermeter-esp32.local)
2. **Configure MQTT** broker in WebUI
3. **Check Home Assistant:**
   - Device "WaterMeter-ESP32" appears
   - 12 entities visible (7 sensors, 2 system, 3 buttons)
   - Values match serial console
4. **Test buttons:**
   - Reset Daily → Counter resets
   - Reset Yearly → Counter resets
   - Restart Device → ESP32 reboots, no false pulses

---

## Troubleshooting

### Problem: No pulses detected

**Possible causes:**
1. Transistor collector/emitter swapped
2. No pull-up resistor (10kΩ to 3.3V)
3. Signal not inverted correctly

**Solution:**
- Check GPIO34 voltage with multimeter:
  - No magnet: Should be ~0V (LOW)
  - Magnet under: Should be ~3.3V (HIGH)
- Verify transistor pinout (E-B-C)

### Problem: Continuous pulses (runaway counter)

**Possible causes:**
1. Short circuit or wiring error
2. Debounce too short
3. Signal noise

**Solution:**
- Disconnect GPIO34 from circuit
- If counter stops → wiring issue
- Add 100nF capacitor for filtering
- Increase `PULSE_DEBOUNCE_MS` to 1000ms in config

### Problem: Main box registers false pulses

**Possible causes:**
1. ESP32 directly connected to sensor (no isolation)
2. Shared ground causing noise
3. Transistor base resistor too small

**Solution:**
- Verify 1kΩ resistor present on transistor base
- Check transistor EMITTER is connected to ESP32 GND (not main box GND)
- Ensure no direct ESP32-sensor connection

### Problem: ESP32 reads opposite values

**This is NORMAL!** Transistor inverts signal:
- Sensor 3.5V → ESP32 reads LOW
- Sensor 0.2V → ESP32 reads HIGH

Code expects this inversion. If readings are correct but detection wrong:
- Change `FALLING` to `RISING` in line 145 of `WaterMeterComponent.h`

### Problem: Pulses counted during boot

**Possible causes:**
1. Boot delay too short
2. Noise during power-up

**Solution:**
- Increase `BOOT_INIT_DELAY_MS` from 3000 to 5000 in `WaterMeterConfig.h`
- Add larger capacitor (220nF or 470nF) between GPIO34 and GND

## Success Checklist

Before declaring installation complete:

- [ ] Breadboard tests all passed
- [ ] MOSFET circuit installed and verified
- [ ] Main control box unaffected by ESP32 power state
- [ ] No false CO2 activation when ESP32 powers on/off
- [ ] Pulses counted accurately (1 liter = 1 pulse)
- [ ] No pulses during 3-second boot protection
- [ ] Reboot with magnet under sensor = no false count
- [ ] WebUI accessible and functional
- [ ] MQTT publishing to Home Assistant (if configured)
- [ ] All 12 entities visible in Home Assistant
- [ ] Serial console commands working
- [ ] LED flashes on each valid pulse

**If all checks pass → Installation successful! 🎉**

---

## Safety Notes

- Low voltage only (3.3V/5V)
- Disconnect power before wiring changes
- Do not modify main control box
- Sensor is high impedance (read-only)
- Data auto-saved to NVS every 30s
