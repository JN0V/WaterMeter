# Pulse Counting Logic & Stability Analysis

## Overview
The WaterMeter project uses a specialized pulse counting algorithm designed to handle "noisy" magnetic reed switch signals, specifically addressing the issue of **exit bounces** during slow magnet passes.

## The Problem: "Double Counting"
Standard debounce logic (wait X ms after pulse) fails when the magnet moves slowly past the sensor or stops near the edge.
1. **Entry**: Magnet triggers sensor (FALLING edge) -> **Pulse Validated**.
2. **Exit**: As the magnet leaves (slowly), the magnetic field fluctuates, causing the reed switch to "bounce" (rapidly switch LOW/HIGH).
3. **Failure Mode**: If these bounces occur *after* the standard debounce time (e.g., >500ms later), a simple debounce algorithm interprets them as a **new pulse**.

## The Solution: High-State Stability Check
We implemented a state-based validation that requires the signal to be **stably HIGH** (magnet away) for a minimum duration before accepting a new **FALLING** edge (magnet arriving).

### Logic
- **Interrupt Mode**: `CHANGE` (monitors both FALLING and RISING edges).
- **Parameters**:
  - `g_pulseDebounceMs` (Default: 500ms): Minimum time between two valid pulses.
  - `g_pulseHighStableMs` (Default: 150ms): Minimum time the signal must be HIGH before a new FALLING edge is accepted.

### Algorithm (ISR)
```cpp
void IRAM_ATTR waterMeterPulseISR() {
    // ... initialization check ...

    if (pinState == LOW) { // FALLING EDGE (Potential Pulse)
        unsigned long timeDiff = currentTime - g_lastPulseTime;
        unsigned long stableHighDiff = currentTime - g_lastRisingTime;
        
        // VALIDATION CONDITIONS:
        // 1. Debounce: Time since last pulse > 500ms
        // 2. Stability: Time since signal went HIGH > 150ms
        if (timeDiff > g_pulseDebounceMs && stableHighDiff > g_pulseHighStableMs) {
            g_pulseCount++;
            // ... valid pulse ...
        }
    } else { // RISING EDGE (Magnet leaving)
        g_lastRisingTime = currentTime;
    }
}
```

## Why this works
When the magnet leaves and causes bounces:
1. The signal goes HIGH (RISING edge) -> `g_lastRisingTime` is updated.
2. A bounce causes a quick LOW (FALLING edge) e.g., 10ms later.
3. Check: `currentTime - g_lastRisingTime` is only 10ms.
4. **Result**: 10ms < 150ms (`g_pulseHighStableMs`), so the pulse is **IGNORED**.

This effectively filters out all "exit noise" regardless of how long after the initial pulse it occurs, provided the noise frequency is higher than 6.6Hz (150ms period), which is true for mechanical contact bounce.
