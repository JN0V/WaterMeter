# Changelog

All notable changes to the WaterMeter project will be documented in this file.

## [0.9.2] - 2025-11-23

### Fixed - Critical Pulse Counting Logic 🧲
- **High-State Stability Check**: Implemented a new validation logic to eliminate false "double counts" caused by sensor exit bounces during slow water flow.
  - **Problem**: Standard debounce failed when magnet exited slowly, causing late bounces (>500ms) to be registered as new pulses.
  - **Solution**: Added `pulseHighStableMs` (150ms) requirement. A pulse is now ONLY validated if the signal was stably HIGH for >150ms prior to the FALLING edge.
  - **Technical**: Switched ISR interrupt mode from `FALLING` to `CHANGE` to track stability.
- **Documentation**: Added detailed technical guide `docs/technical/PULSE_LOGIC.md`.

### Changed
- **Configuration**: Added `pulseHighStableMs` parameter (default 150ms).
- **Structure**: Moved technical documentation to `docs/technical/` and migration guides to `docs/migration/`.
- **Version**: Bumped to 0.9.2.

## [0.9.1] - 2025-11-17
### Added
- Initial implementation of `WaterMeterComponent` with DomoticsCore v1.2.1.
- Support for WebUI configuration.
- MQTT and Home Assistant auto-discovery (EventBus orchestration).

## [0.9.0] - 2025-11-11

### Added - Home Assistant Integration 🏠

**Full Home Assistant auto-discovery with MQTT!**

### Fixed - Code Quality Improvements

#### Code cleanup
- ✅ Changed `delay(100)` to `yield()` in error loop (non-blocking, feeds watchdog)
- ✅ Removed redundant `using namespace DomoticsCore::Components::HomeAssistant;`
- ✅ Simplified component access (removed redundant `Components::` prefix)
- ✅ Fixed namespace usage for `HomeAssistant::HomeAssistantComponent`
- ✅ HomeAssistantWebUI provider automatically registered by System::fullStack()

#### Documentation
- ✅ Translated all French content in CHANGELOG to English

#### Sensors (7 water meter + 2 system)
- ✅ **Total Water Volume** (m³) - `sensor.watermeter_esp32_total_volume`
- ✅ **Total Liters** (L) - `sensor.watermeter_esp32_total_liters`
- ✅ **Daily Consumption** (m³) - `sensor.watermeter_esp32_daily_volume`
- ✅ **Daily Liters** (L) - `sensor.watermeter_esp32_daily_liters`
- ✅ **Yearly Consumption** (m³) - `sensor.watermeter_esp32_yearly_volume`
- ✅ **Yearly Liters** (L) - `sensor.watermeter_esp32_yearly_liters`
- ✅ **Total Pulses** - `sensor.watermeter_esp32_pulse_count`
- ✅ **WiFi Signal** (dBm) - `sensor.watermeter_esp32_wifi_signal`
- ✅ **Uptime** (s) - `sensor.watermeter_esp32_uptime`

#### Buttons (3 controls)
- ✅ **Reset Daily Counter** - `button.watermeter_esp32_reset_daily`
- ✅ **Reset Yearly Counter** - `button.watermeter_esp32_reset_yearly`
- ✅ **Restart Device** - `button.watermeter_esp32_restart`

#### Features
- **Auto-discovery**: All entities appear automatically in Home Assistant
- **Real-time updates**: Published every 30 seconds
- **Device class**: Proper water device class for sensors
- **Icons**: Material Design Icons (mdi) for each entity
- **Remote control**: Reset counters from Home Assistant dashboard

#### Configuration Required
1. Configure MQTT broker in WebUI (Settings → MQTT)
2. Broker IP, port, credentials
3. Home Assistant MQTT integration enabled
4. Entities appear automatically in HA

#### Device Info
```yaml
Device: WaterMeter-ESP32
Manufacturer: DomoticsCore
Model: ESP32-DevKit
Area: (Configurable in HA)
Config URL: http://device_ip
```

#### Code Changes
- `src/main.cpp`:
  - Added HomeAssistant component setup
  - Created 9 sensors + 3 buttons
  - Periodic state publishing (30s)
  - Initial state on HA ready
  - HomeAssistantWebUI provider auto-registered by System::fullStack()
- Added includes:
  - `<DomoticsCore/HomeAssistant.h>`
  - `<DomoticsCore/Timer.h>`

#### Example Usage
**Home Assistant Dashboard:**
```yaml
type: entities
entities:
  - entity: sensor.watermeter_esp32_daily_volume
    name: Today
  - entity: sensor.watermeter_esp32_yearly_volume
    name: This Year
  - entity: sensor.watermeter_esp32_total_volume
    name: Total
  - entity: button.watermeter_esp32_reset_daily
```

**Automation Example:**
```yaml
automation:
  - alias: "Water leak alert"
    trigger:
      - platform: numeric_state
        entity_id: sensor.watermeter_esp32_daily_volume
        above: 0.5  # 500 liters per day
    action:
      - service: notify.mobile_app
        data:
          message: "High water consumption detected!"
```

---

## [0.8.3] - 2025-11-09

### Fixed - Dashboard Refresh & GPIO Isolation

#### WebUI - Real-Time Updates (FIXED)
**Problem:** Dashboard doesn't auto-refresh after override (Ctrl+Shift+R required)

**Cause:** `withRealTime(2000)` was configured but `getWebUIData()` was not implemented

**Solution:**
- ✅ Implemented `getWebUIData(contextId)` for real-time updates
- ✅ Dashboard auto-refreshes every 2s via this hook
- ✅ Settings input syncs with current pulse count every 2s too
- ✅ No need for hard refresh after override

**Code:**
```cpp
String getWebUIData(const String& contextId) override {
    // Called automatically every 2s by DomoticsCore WebUI
    // Returns updated values for each context
}
```

#### Hardware - GPIO Change (IMPORTANT)
**GPIO25 → GPIO32 for STATUS_LED_PIN**

**Problem solved:**
Water meter shares its 2 wires with a CO2 module. When ESP32 is powered off:
- GPIO25 (DAC, strapping pin) pulls to GND or floats
- Signal disturbed → CO2 module stops working

**Solution:**
- GPIO32 has stable high-impedance when ESP32 off
- No DAC/strapping function
- Signal to CO2 undisturbed even if ESP32 off

**Action required:** Move LED from GPIO25 → GPIO32

#### Documentation
- ✅ Deleted `docs/STORAGE_DEPENDENCY_ISSUE_RESOLVED_v1.1.4.md` (obsolete)

---

## [0.8.2] - 2025-11-09

### Fixed - WebUI Corrections
- ✅ Dashboard uses Display fields (read-only, not editable)
- ✅ Removed separator field that displayed incorrectly
- ✅ Fixed override functionality using proper field/value pattern
- ✅ Override now stores value when input changes, applies on button click
- ✅ Cleaned documentation (removed verbose WEBUI_GUIDE.md)

---

## [0.8.1] - 2025-11-09

### Improved - WebUI Redesign & Documentation Cleanup 🎨

#### 🎨 WebUI Improvements
**Dashboard with Real-Time Values**
- ✅ Removed badge (not relevant for water meter)
- ✅ Added dedicated dashboard with current consumption display
  - Total pulses and volume
  - Today's consumption (L and m³)
  - Yearly consumption (L and m³)
  - Auto-refresh every 2 seconds

**Enhanced Settings/Controls**
- ✅ **Override functionality** - Synchronize with physical meter
  - Input field for total pulses
  - Apply button to override counter
  - Useful for initial calibration or sync
- ✅ **Reset controls**
  - Reset daily counter
  - Reset yearly counter
  - Cleaner button layout

**User Experience**
- Dashboard: Read-only display with real-time updates
- Settings: Interactive controls for management
- Clear separation between monitoring and actions

#### 🧹 Documentation Cleanup
**Files Removed:**
- `V1.1.0_BUGS_SUMMARY.md` (obsolete)
- `V1.1.1_SUCCESS.md` (obsolete)
- `V1.1.X_BUGS_FINAL.md` (obsolete)
- `docs/BUGS_CLEANED.md` (temporary)
- `docs/V1.1.4_MIGRATION.md` (too verbose)
- `docs/V1.1.4_SUMMARY.md` (redundant)

**Files Kept:**
- ✅ `README.md` - Main documentation
- ✅ `CHANGELOG.md` - Version history
- ✅ `ARCHITECTURE.md` - System design
- ✅ `docs/circuit_protection.md` - Hardware guide
- ✅ `docs/STORAGE_DEPENDENCY_ISSUE_RESOLVED_v1.1.4.md` - Historical reference

#### Code Changes
- `include/WaterMeterWebUI.h`:
  - Removed `statusBadge` context
  - Added `dashboard` context with all current values
  - Enhanced `settings` with override input field
  - Updated API handlers for new functionality
- `include/WaterMeterComponent.h`:
  - Added `overridePulseCount(uint64_t)` method
  - Saves to storage and logs override action
- `include/WaterMeterConfig.h`:
  - Version: 0.8.0 → 0.8.1

#### WebUI Structure
```
Dashboard (watermeter_dashboard):
  - Total Pulses: 12345
  - Total Volume: 12.345 m³
  - Today: 123 L (0.123 m³)
  - This Year: 5678 L (5.678 m³)
  [Updates every 2s]

Settings (watermeter_settings):
  - Override Total Pulses: [input field]
  - [Apply Override button]
  ---
  - [Reset Daily Counter button]
  - [Reset Yearly Counter button]
```

#### Use Cases
- **Dashboard**: Monitor consumption in real-time
- **Override**: Sync with physical meter reading
- **Reset**: Clear daily/yearly counters manually

---

## [0.8.0] - 2025-11-09

### Upgraded - DomoticsCore v1.1.4 Integration 🎉
**Storage Dependency Fix - Workarounds Removed!**

#### ✅ Breaking Changes from DomoticsCore v1.1.4
- **Unified Metadata Initialization Pattern**
  - Components now initialize `metadata` in constructor (not in `begin()`)
  - Removed `getName()` and `getVersion()` virtual methods
  - Direct access via `component->metadata.name` instead
  - Metadata available immediately for dependency resolution

#### 🐛 Bug Fixed Upstream
- **Storage Dependency Declaration** - ✅ **FIXED in v1.1.4!**
  - Storage now initializes `metadata.name` in constructor
  - Can now be declared in `getDependencies()`
  - Proper initialization order via topological sort
  - No more workarounds needed!

#### 🧹 Workarounds Removed
- ✅ Removed `afterAllComponentsReady()` retry pattern
- ✅ Removed empty name access `getComponent<Storage>("")`
- ✅ Storage now accessed by proper name: `getComponent<Storage>("Storage")`
- ✅ Storage declared in `getDependencies()` as optional dependency
- ✅ Single load in `begin()` - works correctly on first try

#### Code Changes
- `include/WaterMeterComponent.h`:
  - Removed `getName()` override (use `metadata.name`)
  - Added Storage to `getDependencies()`
  - Removed `afterAllComponentsReady()` workaround
  - Use `"Storage"` instead of `""` in getComponent calls
- `include/WaterMeterWebUI.h`:
  - Use `waterMeter->metadata.name` instead of `getName()`
  - Use `waterMeter->metadata.version` instead of `getVersion()`
- `include/WaterMeterConfig.h`:
  - Version: 0.7.1 → 0.8.0

#### Technical Details
**Before v1.1.4:**
```cpp
StorageComponent() {
    // ❌ metadata.name not set
}
ComponentStatus begin() {
    metadata.name = "Storage";  // Too late!
}
```

**After v1.1.4:**
```cpp
StorageComponent() {
    metadata.name = "Storage";  // ✅ Immediate!
    // ...
}
```

**Benefits:**
- ✅ Clean dependency declaration
- ✅ Proper initialization order
- ✅ No workarounds needed
- ✅ Consistent with DomoticsCore patterns

#### Files Modified
- `include/WaterMeterComponent.h` - Removed workarounds, cleaner code
- `include/WaterMeterWebUI.h` - Updated for v1.1.4 API
- `include/WaterMeterConfig.h` - Version bump
- `CHANGELOG.md` - This entry

#### Documentation Status
- `docs/STORAGE_DEPENDENCY_ISSUE.md` - Now historical reference
- Bug fixed in DomoticsCore v1.1.4
- Workarounds no longer necessary

---

## [0.7.1] - 2025-11-07

### Documentation & Code Cleanup 📚

#### 🧹 Files Cleaned
- Removed temporary bug tracking files (`V1.1.*.md`)
- Cleaned up version-specific comments in code
- Simplified workaround comments with references to documentation

#### 📝 Documentation Added
- **`docs/STORAGE_DEPENDENCY_ISSUE.md`** - Complete analysis of Storage dependency problem
  - Root cause explanation
  - Technical sequence breakdown
  - Workaround pattern documentation
  - Proposed fix for DomoticsCore
- **`docs/BUGS_CLEANED.md`** - Summary of cleanup actions

#### 🐛 Bug Report Upstream
- Reported Storage dependency issue in DomoticsCore Roadmap.md
- Severity: MEDIUM (workaround available)
- Proposed fix: Initialize `metadata.name` in constructor

#### Code Changes
- `include/WaterMeterComponent.h` - Cleaned comments
  - Removed version references (v1.1.1)
  - Added documentation references
  - Kept functional comments only

#### Why Storage Cannot Be Declared as Dependency

**Technical Explanation:**
Storage's `metadata.name` is set in `begin()`, not in constructor. This means:
1. Storage added to registry with empty name ("")
2. Dependency resolution builds map by name → Storage not found
3. Storage::begin() sets name → too late, dependencies already resolved

**Current Workaround:**
- Access by empty name: `getComponent<StorageComponent>("")`
- Double-load pattern: attempt in `begin()` + retry in `afterAllComponentsReady()`
- Cannot declare in `getDependencies()`

**See:** `docs/STORAGE_DEPENDENCY_ISSUE.md` for full details

---

## [0.7.0] - 2025-11-01

### Added - WebUI Interface 🎨
**Complete Web Interface for WaterMeter!**

#### ✨ New Features
- **Dashboard Badge** - Real-time consumption display in header
  - Shows total volume (m³) and daily consumption (L)
  - Auto-refresh every 5 seconds
- **Settings Page** - Complete monitoring and control interface
  - View all counters (pulses, total, daily, yearly)
  - Reset daily counter button
  - Reset yearly counter button
  - Real-time data updates

#### User Interface
Access via DomoticsCore WebUI at http://192.168.4.1:
- Header badge shows current water consumption
- Settings page: `/` → Water Meter section
- API endpoints:
  - `/api/watermeter/status` - Current status JSON
  - `/api/watermeter/settings` - Full data + controls

#### Implementation
- Custom `WaterMeterWebUIProvider` implementing `IWebUIProvider`
- Integrated with DomoticsCore WebUI system
- Responsive design using DomoticsCore UI components
- JSON API for external monitoring

#### Files Added
- `include/WaterMeterWebUI.h` - WebUI provider implementation

#### Files Modified
- `src/main.cpp` - WebUI provider registration
- `include/WaterMeterConfig.h` - Version: 0.6.3 → 0.7.0
- `CHANGELOG.md` - This entry
- `README.md` - WebUI documentation

---

## [0.6.3] - 2025-11-01

### Added - DomoticsCore v1.1.3 Integration
**Storage Namespace Isolation! 🎯**

#### ✨ New Feature
- **Custom storage namespace** - Configured as `"watermeter"`
  - Isolated NVS namespace for this project
  - No conflicts with other DomoticsCore projects on same device
  - Easier firmware testing without data contamination

#### Configuration
```cpp
SystemConfig config = SystemConfig::fullStack();
config.storageNamespace = "watermeter";  // Custom namespace
```

#### Benefits
- ✅ Multiple DomoticsCore projects can coexist on same ESP32
- ✅ Clean separation between project data
- ✅ Easier firmware testing and development
- ✅ Backward compatible (migration needed from "domotics" namespace)

#### Files Modified
- `src/main.cpp` - Added `storageNamespace` configuration
- `include/WaterMeterConfig.h` - Version: 0.6.2 → 0.6.3
- `CHANGELOG.md` - This entry
- `README.md` - Updated to v1.1.3

#### Note
⚠️ **Data Migration:** Existing data in "domotics" namespace won't be automatically migrated. If you have stored counter values, you'll need to manually note them and reset after upgrade, or copy NVS partitions.

---

## [0.6.2] - 2025-11-01

### Fixed - DomoticsCore v1.1.2 Integration
**AP Mode Bug FIXED! 🎉**

#### 🐛 Bug Fixed Upstream
- **AP mode regression** - ✅ Fixed in v1.1.2
  - WifiComponent AP mode broken by begin() mode override
  - AP now starts correctly with IP 192.168.4.1

#### ✅ All v1.1.x Features Now Working
- ✅ Optional dependencies functional
- ✅ Lifecycle callbacks functional  
- ✅ AP mode working (SSID: WaterMeter-ESP32-XXXX, open network)
- ✅ Storage accessible via type-only lookup workaround

#### Files Modified
- `include/WaterMeterConfig.h` - Version: 0.6.1 → 0.6.2
- `CHANGELOG.md` - This entry
- `README.md` - Updated to v1.1.2

---

## [0.6.1] - 2025-10-31

### Fixed - DomoticsCore v1.1.1 Integration
**Both v1.1.0 Critical Bugs RESOLVED! 🎉**

#### 🐛 Bugs Fixed Upstream
1. **MQTT.h signature** - ✅ Fixed in v1.1.1
2. **Storage early-init crash** - ✅ Fixed in v1.1.1

#### ⚠️ Known Issue (v1.1.1)
3. **setCredentials() ambiguity** - Local workaround applied in System.h
   - Bug reported in `../DomoticsCore/Roadmap.md`
   - Temporary fix: `wifi->setCredentials(ssid, password, true)`

#### 🎯 What Works Now
- ✅ **Optional dependencies fully functional!**
  ```cpp
  std::vector<Dependency> getDependencies() const override {
      return {
          {"Storage", false},  // Works perfectly now!
          {"NTP", false}
      };
  }
  ```
- ✅ **Framework logs missing optional deps** (INFO level)
- ✅ **No more crashes** - Storage initialized in normal order
- ✅ **Lifecycle callback** works as designed

#### Technical Details
**v1.1.0 Problem:**
- Storage registered as "early-init" DURING `begin()`
- Custom components registered BEFORE `begin()` couldn't declare Storage dependency
- Result: Crash at `ComponentRegistry::initializeAll()`

**v1.1.1 Solution:**
- Storage early-init pattern eliminated
- All components (except LED) initialized in dependency order
- WiFi can now wait for Storage to load credentials
- Optional dependencies on built-ins work correctly

#### Files Modified
- `include/WaterMeterComponent.h`:
  - **RESTORED** optional dependencies declaration ✅
  - Removed workaround comments
  - Updated to reference v1.1.1+
- `include/WaterMeterConfig.h` - Version: 0.6.0 → 0.6.1
- `CHANGELOG.md` - This entry
- `README.md` - Updated to v1.1.1+

### Code Quality
- ✅ **All v1.1.0 features now working as designed**
- ✅ **Explicit optional dependencies** (intent clear)
- ✅ **Clean lifecycle separation** (GPIO vs deps)
- ✅ **No workarounds needed** (framework handles everything)

---

## [0.6.0] - 2025-10-31

### Added - DomoticsCore v1.1.0 Integration
**Optional Dependencies + Lifecycle Callback!**

#### 🎯 What's New
- ✅ **Upgraded to DomoticsCore v1.1.0** with optional dependencies and lifecycle callback
- ✅ **Dependencies declared**: Storage and NTP now properly declared as optional dependencies
- ✅ **Lifecycle separation**: `begin()` for GPIO init, `afterAllComponentsReady()` for dependency access
- ✅ **Cleaner code**: Intent is explicit, framework handles component availability

#### API Changes
**getDependencies() - Now with Optional Flag:**
```cpp
// Before (v0.5.2)
std::vector<String> getDependencies() const override {
    return {};  // Couldn't declare built-ins
}

// After (v0.6.0)
std::vector<Dependency> getDependencies() const override {
    return {
        {"Storage", false},  // Optional - explicit intent!
        {"NTP", false}
    };
}
```

**afterAllComponentsReady() - New Lifecycle Hook:**
```cpp
// Before (v0.5.2) - Mixed initialization
ComponentStatus begin() override {
    pinMode(PIN, INPUT);    // GPIO
    loadFromStorage();      // Dependency access - risky!
    return Success;
}

// After (v0.6.0) - Clean separation
ComponentStatus begin() override {
    pinMode(PIN, INPUT);    // GPIO only
    return Success;
}

void afterAllComponentsReady() override {
    loadFromStorage();      // Safe - all components ready!
}
```

#### Benefits
- **Explicit intent**: Dependencies clearly declared with optional flag
- **Better lifecycle**: Separate GPIO init from dependency access
- **Framework support**: Logs INFO for missing optional deps
- **Defensive checks still needed**: Optional deps may be unavailable

#### Known Issues - v1.1.0 CRITICAL BUGS

**Bug #1: MQTT.h not updated**
- ⚠️ File: `DomoticsCore-MQTT/include/DomoticsCore/MQTT.h:151`
- Fix: Change `std::vector<String>` to `std::vector<Dependency>`
- Workaround: Manual edit required
- Status: Reported

**Bug #2: Optional Dependencies + Early-Init Components = CRASH**
- 🚨 **CRITICAL**: Declaring Storage/NTP as optional dependencies causes boot crash
- Error: `LoadProhibited` at `ComponentRegistry.h:95`
- Root cause: Early-init components not in registry when dependencies resolved
- **Workaround applied**: Reverted to empty `getDependencies()`, kept `afterAllComponentsReady()`
- Benefits lost: Cannot explicitly declare optional built-in dependencies
- Benefits kept: `afterAllComponentsReady()` lifecycle still works!
- Status: Reported with detailed analysis

#### Files Modified
- `include/WaterMeterComponent.h`:
  - ~~`getDependencies()` declares Storage and NTP as optional~~ **REVERTED due to bug #2**
  - `getDependencies()` returns `{}` (workaround for crash)
  - Added `afterAllComponentsReady()` for Storage loading ✅ (this still works!)
  - Moved `loadFromStorage()` call from `begin()` to `afterAllComponentsReady()`
  - Updated comments to document v1.1.0 bugs
- `include/WaterMeterConfig.h` - Version: 0.5.2 → 0.6.0
- `CHANGELOG.md` - This entry
- `README.md` - Updated requirement to v1.1.0+
- `ARCHITECTURE.md` - Documented new lifecycle pattern

### Code Quality vs v0.5.2
- ✅ **Improved**: Cleaner separation (GPIO in `begin()`, deps in `afterAllComponentsReady()`)
- ❌ **Regressed**: Cannot declare optional dependencies (v1.1.0 bug)
- ⚠️ **Partial**: Uses `afterAllComponentsReady()` callback (works!) but not optional deps (broken)

---

## [0.5.2] - 2025-10-30

### Changed - DomoticsCore v1.0.2 Integration
**Lazy Core Injection - Ultimate Flexibility!**

#### 🎯 What's New
- ✅ **Upgraded to DomoticsCore v1.0.2** with lazy Core injection
- ✅ **Registration order no longer matters** - components can be added before OR after `begin()`
- ✅ **Zero risk of crashes** - `getCore()` uses lazy injection via `__dc_registry`
- ✅ **Pattern unchanged** - our defensive checks are still correct and recommended

#### Technical Details
**How lazy injection works:**
1. `__dc_registry` injected immediately when component is registered
2. `getCore()` fetches Core from registry on first access (if not already cached)
3. Single `if` check overhead per call (~1 nanosecond)
4. +4 bytes memory per component for registry pointer

**Our code already followed v1.0.2 best practices:**
- ✅ `getDependencies()` returns `{}` for built-ins (correct)
- ✅ Defensive null checks in `loadFromStorage()`, `saveToStorage()`, `checkTimeBasedResets()` (correct)
- ✅ No crashes possible with new lazy injection

#### Files Updated
- `include/WaterMeterComponent.h` - Updated comments to reference v1.0.2 pattern
- `src/main.cpp` - Clarified that registration order is now flexible
- `ARCHITECTURE.md` - Explained lazy injection benefits
- `platformio.ini` - Automatic update to v1.0.2 via GitHub dependency

### No Code Changes Required! 🎉
Our implementation was already using the recommended pattern. Only documentation/comments updated.

---

## [0.5.1] - 2025-10-30

### Fixed - Critical
- ⚠️ **Component registration order**: Custom components must be added **BEFORE** `domotics->begin()`
  - Crash fixed: `LoadProhibited` at `ComponentRegistry::initializeAll()`
  - Root cause: Core injection happens during `begin()`, components added after can't receive Core pointer
  - Solution: Move `addComponent()` call before `begin()`

### Changed - DomoticsCore v1.0.1 Integration
**Major code simplification thanks to DomoticsCore v1.0.1 improvements!**

#### Core Access Simplification
- ✅ **REMOVED** manual `Core*` pointer in component
- ✅ **REMOVED** `setCore()` boilerplate method
- ✅ **REMOVED** manual Core injection in main.cpp
- ✅ **ADDED** automatic Core injection via `__dc_core`
- Components now use built-in `getCore()` method

**Before (v1.0.0):**
```cpp
class WaterMeterComponent {
    Core* core_ = nullptr;
public:
    void setCore(Core* c) { core_ = c; }
};

// In main.cpp
waterMeter->setCore(&domotics->getCore());
```

**After (v1.0.1):**
```cpp
class WaterMeterComponent {
    // No Core pointer needed!
};

// In main.cpp - just add component
domotics->getCore().addComponent(...);

// Use getCore() anywhere in component
auto* storage = getCore()->getComponent<StorageComponent>("Storage");
```

#### Storage uint64_t Native Support
- ✅ **REMOVED** complex blob + memcpy workaround (8 lines → 1 line)
- ✅ **ADDED** native `putULong64()` / `getULong64()` methods
- ✅ **REMOVED** volatile pointer casting issues

**Before (v1.0.0):**
```cpp
// Complex workaround (21 lines)
void saveToStorage() {
    uint8_t buffer[8];
    memcpy(buffer, (const void*)&g_pulseCount, 8);
    storage->putBlob("pulse_count", buffer, 8);
    // Repeat for each variable...
}
```

**After (v1.0.1):**
```cpp
// Clean and simple (3 lines)
void saveToStorage() {
    storage->putULong64("pulse_count", g_pulseCount);
    storage->putULong64("daily_liters", dailyLiters);
}
```

### Code Metrics
- **Lines removed:** 25 lines (boilerplate eliminated)
- **Complexity:** Significantly reduced
- **Readability:** Much improved
- **Memory usage:** Unchanged (14.8% RAM, 70.8% Flash)

### Files Modified
- `include/WaterMeterComponent.h` - Removed Core* boilerplate, simplified Storage calls
- `src/main.cpp` - Removed manual setCore() injection
- `ARCHITECTURE.md` - Updated to reflect v1.0.1 patterns
- `platformio.ini` - Already using #main (tracks v1.0.1+)

---

## [0.5.0] - 2025-10-30

### Added - Initial DomoticsCore v1.0 Integration
- ✅ Full migration from DomoticsCore v0.2 to v1.0
- ✅ Custom `WaterMeterComponent` implementing `IComponent` interface
- ✅ ISR pulse counting (global function pattern for ESP32 IRAM)
- ✅ NVS storage persistence (auto-save every 30s)
- ✅ NTP-based auto-resets (daily at midnight, yearly Jan 1st)
- ✅ Event bus data publishing (every 5s)
- ✅ Non-blocking LED feedback (50ms pulse flash)
- ✅ Console commands: `water`, `reset_daily`, `reset_yearly`
- ✅ Full documentation in ARCHITECTURE.md

### Technical Details
- **Framework:** DomoticsCore v1.0 with System::fullStack()
- **Components Used:** WiFi, WebUI, MQTT, Storage, NTP, OTA, LED, Console
- **ISR Pattern:** Global function to avoid ESP32 IRAM linker issues
- **Storage:** Blob-based uint64_t persistence (workaround for v1.0.0)
- **Timers:** 3 NonBlockingDelay instances (LED, save, publish)

### Hardware
- **ESP32 Dev Board**
- **GPIO34:** Pulse input (magnetic sensor, FALLING edge)
- **GPIO25:** Status LED (pulse indicator)
- **GPIO2:** System LED (DomoticsCore status)

### Memory Usage
- **RAM:** 14.8% (48,376 bytes)
- **Flash:** 70.6% (1,388,785 bytes)

### Files Created
- `src/main.cpp` - Application entry point (92 lines)
- `include/WaterMeterComponent.h` - Core water meter logic (290 lines → 265 lines in v0.5.1)
- `include/WaterMeterConfig.h` - Hardware configuration
- `ARCHITECTURE.md` - Complete technical documentation
- `README.md` - User guide

### Dependencies
- DomoticsCore v1.0+ from GitHub
- ArduinoJson 7.0+
- ESPAsyncWebServer 3.8+
- AsyncTCP 3.4+
- PubSubClient 2.8+

---

## Version History

- **v0.5.1** (2025-10-30): DomoticsCore v1.0.1 integration - major code simplification
- **v0.5.0** (2025-10-30): Initial DomoticsCore v1.0 integration
- **v0.2.x** (Earlier): Original implementation with DomoticsCore v0.2

---

## Impact Summary

### Code Quality Improvements (v0.5.0 → v0.5.1)
| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| WaterMeterComponent.h | 290 lines | 265 lines | -25 lines (-8.6%) |
| Core boilerplate | 6 lines | 0 lines | -6 lines (eliminated) |
| Storage calls | Complex (memcpy) | Simple (native) | Much cleaner |
| Null checks | 3 per method | 1 per method | -66% |
| Readability | Good | Excellent | Significantly improved |

### Developer Experience
- ⚡ **Faster:** Less boilerplate to write
- 🎯 **Clearer:** Obvious patterns, no magic
- 🐛 **Safer:** Less manual memory management
- 📚 **Easier:** Better aligned with framework conventions

### Thanks
Special thanks to DomoticsCore maintainer for implementing the suggested API improvements from our feedback in Roadmap.md!
