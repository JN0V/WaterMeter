#ifndef WATER_METER_WEBUI_H
#define WATER_METER_WEBUI_H

#include <DomoticsCore/IWebUIProvider.h>
#include <DomoticsCore/WebUI.h>
#include <DomoticsCore/BaseWebUIComponents.h>
#include <ArduinoJson.h>
#include "WaterMeterComponent.h"

using namespace DomoticsCore;
using namespace DomoticsCore::Components::WebUI;

/**
 * @brief WebUI Provider for WaterMeter Component
 */
class WaterMeterWebUIProvider : public IWebUIProvider {
private:
    WaterMeterComponent* waterMeter;
    
public:
    explicit WaterMeterWebUIProvider(WaterMeterComponent* wm) : waterMeter(wm) {}
    
    String getWebUIName() const override { 
        return waterMeter ? waterMeter->metadata.name : String("WaterMeter"); 
    }
    
    String getWebUIVersion() const override { 
        return waterMeter ? waterMeter->metadata.version : String(WATER_METER_VERSION); 
    }
    
    String getWebUIData(const String& contextId) override {
        if (!waterMeter) return "{}";
        
        WaterMeterData data = waterMeter->getData();
        JsonDocument doc;
        
        if (contextId == "watermeter_dashboard") {
            // Real-time dashboard updates (called every 60s)
            // Use char buffers to avoid String allocations/concatenations
            char totalBuf[32];
            char dailyBuf[64];
            char yearlyBuf[64];
            
            snprintf(totalBuf, sizeof(totalBuf), "%.3f m³", data.totalM3);
            snprintf(dailyBuf, sizeof(dailyBuf), "%llu L (%.3f m³)", data.dailyLiters, data.dailyM3);
            snprintf(yearlyBuf, sizeof(yearlyBuf), "%llu L (%.3f m³)", data.yearlyLiters, data.yearlyM3);
            
            doc["pulse_count"] = data.pulseCount;
            doc["total_m3"] = totalBuf;
            doc["daily_liters"] = dailyBuf;
            doc["yearly_liters"] = yearlyBuf;
        }
        else if (contextId == "watermeter_settings") {
            // Update all input fields with current values
            doc["total_pulses"] = data.pulseCount;
            doc["daily_liters"] = data.dailyLiters;
            doc["yearly_liters"] = data.yearlyLiters;
        }
        
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    std::vector<WebUIContext> getWebUIContexts() override {
        std::vector<WebUIContext> contexts;
        if (!waterMeter) return contexts;
        
        WaterMeterData data = waterMeter->getData();
        
        // Dashboard - Current Values (read-only display)
        WebUIContext dashboard = WebUIContext::dashboard("watermeter_dashboard", "Water Consumption");
        dashboard.withField(WebUIField("pulse_count", "Total Pulses", WebUIFieldType::Display, "", "", true))
                 .withField(WebUIField("total_m3", "Total Volume", WebUIFieldType::Display, "", "", true))
                 .withField(WebUIField("daily_liters", "Today", WebUIFieldType::Display, "", "", true))
                 .withField(WebUIField("yearly_liters", "This Year", WebUIFieldType::Display, "", "", true))
                 .withRealTime(60000)  // Update every 60s (water consumption changes slowly)
                 .withAPI("/api/watermeter/dashboard");
        contexts.push_back(dashboard);
        
        // Settings/Controls - Edit all counters
        WebUIContext settings = WebUIContext::settings("watermeter_settings", "Water Meter Controls");
        settings.withField(WebUIField("total_pulses", "Total Pulses", WebUIFieldType::Number, ""))
                .withField(WebUIField("daily_liters", "Daily Liters", WebUIFieldType::Number, ""))
                .withField(WebUIField("yearly_liters", "Yearly Liters", WebUIFieldType::Number, ""))
                .withRealTime(60000)  // Sync input fields every 60s
                .withAPI("/api/watermeter/settings");
        contexts.push_back(settings);
        
        return contexts;
    }
    
    String handleWebUIRequest(const String& contextId, const String& endpoint,
                             const String& method, const std::map<String, String>& params) override {
        if (!waterMeter) {
            return "{\"success\":false}";
        }
        
        // GET requests - use getWebUIData for consistency
        if (method == "GET") {
            return getWebUIData(contextId);
        }
        
        // POST requests - handle actions
        if (method == "POST" && contextId == "watermeter_settings") {
            auto fieldIt = params.find("field");
            auto valueIt = params.find("value");
            
            if (fieldIt != params.end() && valueIt != params.end()) {
                const String& field = fieldIt->second;
                const String& value = valueIt->second;
                
                // Apply overrides immediately when fields change (Edit/Save pattern)
                if (field == "total_pulses") {
                    uint64_t newValue = strtoull(value.c_str(), nullptr, 10);
                    waterMeter->overridePulseCount(newValue);
                    return "{\"success\":true}";
                }
                
                if (field == "daily_liters") {
                    uint64_t newValue = strtoull(value.c_str(), nullptr, 10);
                    waterMeter->overrideDailyLiters(newValue);
                    return "{\"success\":true}";
                }
                
                if (field == "yearly_liters") {
                    uint64_t newValue = strtoull(value.c_str(), nullptr, 10);
                    waterMeter->overrideYearlyLiters(newValue);
                    return "{\"success\":true}";
                }
            }
        }
        
        return "{\"success\":false}";
    }
};

#endif // WATER_METER_WEBUI_H
