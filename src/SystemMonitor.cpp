#include "SystemMonitor.h"
#include "MQTTManager.h"
#include "PreferencesManager.h"
#include "config.h"

// Add the include for reset reason functionality
#include "esp_system.h"
#include "rom/rtc.h"  // This header includes rtc_get_reset_reason function

SystemMonitor::SystemMonitor(MQTTManager* mqttManager) 
    : _mqttManager(mqttManager)
    , _lastPublishTime(0)
    , _startupTime(millis())
    , _resetCount(0)
    , _lastSuccessfulNtpSync(0)
    , _ntpSyncAttempts(0)
    , _ntpSyncSuccesses(0)
    , _ntpSyncFailures(0) {
    
    // Get the reset reason
    _resetReason = rtc_get_reset_reason(0);  // CPU 0
    
    // Load the reset count from persistent storage
    loadResetCount();
}

void SystemMonitor::begin() {
    // Increment and save reset count
    _resetCount++;
    saveResetCount();
    
    // Publish online status
    publishStatus(true);
    
    // Publish initial diagnostic data
    publishDiagnostics(true);
}

void SystemMonitor::update() {
    unsigned long now = millis();
    
    // Publish diagnostics periodically
    if (now - _lastPublishTime >= PUBLISH_INTERVAL) {
        Serial.println("[MONITOR] Publishing periodic diagnostics");
        publishDiagnostics(true);
        _lastPublishTime = now;
        
        // Check if we need to publish discovery information
        static bool discoveryPublished = false;
        if (!discoveryPublished && _mqttManager && _mqttManager->connected()) {
            Serial.println("[MONITOR] Publishing initial diagnostic entities discovery");
            publishHomeAssistantDiscovery();
            discoveryPublished = true;
            publishDiagnostics(true);
        }
    }
    
    // Periodic discovery refresh
    if (lastDiscoveryTime > 0 && now - lastDiscoveryTime >= 3600000) { // Every hour
        if (_mqttManager && _mqttManager->connected()) {
            Serial.println("[MONITOR] Refreshing diagnostic entities discovery");
            publishHomeAssistantDiscovery();
        }
    }
}

void SystemMonitor::recordNtpSyncAttempt(bool success) {
    _ntpSyncAttempts++;
    
    if (success) {
        _ntpSyncSuccesses++;
        _lastSuccessfulNtpSync = millis();
    } else {
        _ntpSyncFailures++;
    }
    
    // Optionally publish immediately on change
    // publishDiagnostics(true);
}

bool SystemMonitor::checkMemory() {
    size_t freeHeap = esp_get_free_heap_size();
    if (freeHeap < CRITICAL_MEMORY_THRESHOLD) {
        // Emergency publish with warning flag
        publishMemoryWarning(freeHeap, true);
        return true;
    }
    return false;
}

void SystemMonitor::monitorTaskStacks(TaskHandle_t* taskHandles, const char** taskNames, size_t numTasks) {
    publishTaskStacks(taskHandles, taskNames, numTasks, true);
}

const char* SystemMonitor::getResetReasonString() const {
    switch (_resetReason) {
        case 1: return "Power-on";
        case 3: return "Software reset";
        case 4: return "Legacy watch dog reset";
        case 5: return "Deep Sleep reset";
        case 6: return "Reset by SLC module";
        case 7: return "Timer Group 0 Watch dog reset";
        case 8: return "Timer Group 1 Watch dog reset";
        case 9: return "RTC Watch dog reset";
        case 10: return "Intrusion reset";
        case 11: return "Time Group reset CPU";
        case 12: return "Software reset CPU";
        case 13: return "RTC Watch dog Reset CPU";
        case 14: return "External reset";
        case 15: return "Brownout reset";
        case 16: return "SDIO reset";
        default: return "Unknown";
    }
}

void SystemMonitor::publishDiagnostics(bool retain) {
    if (!_mqttManager || !_mqttManager->connected()) {
        return;
    }
    
    // Create JSON document
    StaticJsonDocument<512> doc;
    
    // Memory statistics
    size_t freeHeap = esp_get_free_heap_size();  // Define freeHeap here
    size_t minFreeHeap = esp_get_minimum_free_heap_size();
    
    doc["free_heap"] = freeHeap;  // Use the variable
    doc["min_free_heap"] = minFreeHeap;
    doc["heap_fragmentation"] = 100 - (freeHeap * 100) / ESP.getHeapSize();  // Now freeHeap is defined
    
    // Rest of the function remains the same
    doc["reset_reason"] = getResetReasonString();
    doc["reset_count"] = _resetCount;
    
    // Uptime
    unsigned long uptime = millis() - _startupTime;
    doc["uptime_ms"] = uptime;
    doc["uptime_hours"] = uptime / 3600000.0;
    
    // System information
    doc["sdk_version"] = ESP.getSdkVersion();
    doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
    
    // Add NTP statistics
    doc["ntp_sync_attempts"] = _ntpSyncAttempts;
    doc["ntp_sync_successes"] = _ntpSyncSuccesses;
    doc["ntp_sync_failures"] = _ntpSyncFailures;
    
    // Calculate time since last successful sync in seconds
    if (_lastSuccessfulNtpSync > 0) {
        unsigned long syncAge = (millis() - _lastSuccessfulNtpSync) / 1000; // Convert to seconds
        doc["ntp_last_sync_age_sec"] = syncAge;
        doc["ntp_last_sync_age_hours"] = syncAge / 3600.0;
    } else {
        doc["ntp_last_sync_age_sec"] = -1; // -1 indicates no successful sync yet
        doc["ntp_last_sync_age_hours"] = -1;
    }
    
    // Get current time info if available
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[20];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d %02d/%02d", 
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                timeinfo.tm_mday, timeinfo.tm_mon + 1);
        doc["current_time"] = timeStr;
    } else {
        doc["current_time"] = "unavailable";
    }
    
    String payload;
    serializeJson(doc, payload);

    Serial.println("[MONITOR] Publishing diagnostics to MQTT");
    
    // Use MQTT_CLIENT_ID consistently, not a dynamically generated ID
    String topic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_AUX_DISPLAY) + "/diagnostics";
    
    Serial.printf("[MONITOR] Publishing to topic: %s\n", topic.c_str());
    _mqttManager->publish(topic.c_str(), payload.c_str(), retain);
}

void SystemMonitor::publishMemoryWarning(size_t freeHeap, bool retain) {
    if (!_mqttManager || !_mqttManager->connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["free_heap"] = freeHeap;
    doc["min_free_heap"] = esp_get_minimum_free_heap_size();
    doc["uptime_ms"] = millis() - _startupTime;
    doc["warning"] = "Low memory";
    
    String payload;
    serializeJson(doc, payload);
    
    // Use consistent topic construction
    String topic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_AUX_DISPLAY) + "/warnings";
    
    Serial.printf("[MONITOR] Publishing to topic: %s\n", topic.c_str());
    Serial.println(payload); 
    _mqttManager->publish(topic.c_str(), payload.c_str(), retain);
}

void SystemMonitor::publishTaskStacks(TaskHandle_t* taskHandles, const char** taskNames, size_t numTasks, bool retain) {
    if (!_mqttManager || !_mqttManager->connected()) {
        return;
    }

    StaticJsonDocument<512> doc;
    
    for (size_t i = 0; i < numTasks; i++) {
        if (taskHandles[i]) {
            doc[taskNames[i]] = uxTaskGetStackHighWaterMark(taskHandles[i]);
        }
    }
    
    String payload;
    serializeJson(doc, payload);
    
    // Use consistent topic construction
    String topic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_AUX_DISPLAY) + "/system/tasks";
    
    Serial.printf("[MONITOR] Publishing to topic: %s\n", topic.c_str());
    Serial.println(payload); 
    _mqttManager->publish(topic.c_str(), payload.c_str(), retain);
}

void SystemMonitor::publishStatus(bool online) {
    if (!_mqttManager || !_mqttManager->connected()) {
        return;
    }
    
    // Use consistent topic construction
    String statusTopic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_AUX_DISPLAY) + "/status";
    
    // Publish 'online' or 'offline' status with retain flag
    const char* status = online ? "online" : "offline";
    _mqttManager->publish(statusTopic.c_str(), status, true);
    
    Serial.printf("[MONITOR] Published status '%s' to: %s\n", status, statusTopic.c_str());
}

void SystemMonitor::loadResetCount() {
    // Use your storage mechanism to load the reset count
    // This example assumes you have a PreferencesManager class
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    // For example only - you'll need to adapt this to your actual preferences system
    // _resetCount = prefs.getUInt("reset_count", 0);
    
    // TODO: Replace with your actual implementation
    _resetCount = 0;  // Default if not found
}

void SystemMonitor::saveResetCount() {
    // Use your storage mechanism to save the reset count
    // This example assumes you have a PreferencesManager class
    // For example only - you'll need to adapt this to your actual preferences system
    
    // TODO: Replace with your actual implementation
    // prefs.putUInt("reset_count", _resetCount);
}
bool SystemMonitor::publishHomeAssistantDiscovery() {
    if (!_mqttManager || !_mqttManager->connected()) {
        Serial.println("[MONITOR] Cannot publish discovery - not connected to MQTT");
        return false;
    }
    
    Serial.println("[MONITOR] Publishing diagnostic sensors to Home Assistant");
    
    // Metrics to register - standard metrics and NTP metrics
    const char* metrics[][4] = {
        // Format: internal name, display name, unit, device class
        {"free_heap", "Free Heap", "bytes", ""},
        {"uptime_hours", "Uptime", "h", "duration"},
        {"ntp_sync_attempts", "NTP Sync Attempts", "", ""},
        {"ntp_sync_successes", "NTP Sync Successes", "", ""},
        {"ntp_sync_failures", "NTP Sync Failures", "", ""},
        {"ntp_last_sync_age_hours", "NTP Last Sync Age", "h", "duration"},
        {"heap_fragmentation", "Heap Fragmentation", "%", ""}
    };
    
    int numMetrics = sizeof(metrics) / sizeof(metrics[0]);
    Serial.printf("[MONITOR] Publishing discovery for %d metrics\n", numMetrics);
    
    bool success = true;
    
    for (int i = 0; i < numMetrics; i++) {
        // Create a unique ID with version suffix
        char uniqueId[64];
        sprintf(uniqueId, "%s_%s_v3", MQTT_CLIENT_ID, metrics[i][0]);
        
        // Create the discovery topic - use the same format as temperature/humidity
        char discoveryTopic[128];
        sprintf(discoveryTopic, "chaoticvolt/sensorhub1/sensor/%s/config", uniqueId);
        
        // Create a JSON document for this metric
        StaticJsonDocument<512> doc;
        
        // Set up device information
        JsonObject device = doc.createNestedObject("device");
        JsonArray identifiers = device.createNestedArray("identifiers");
        identifiers.add(MQTT_CLIENT_ID);
        
        device["name"] = MQTT_CLIENT_ID;
        device["mdl"] = FIRMWARE_VERSION;
        device["mf"] = "chaoticvolt";
        
        // Set entity attributes
        doc["name"] = String(MQTT_CLIENT_ID) + " " + metrics[i][1];
        doc["uniq_id"] = uniqueId;
        
        // Set state topic to the diagnostics topic
        char stateTopic[128];
        sprintf(stateTopic, "chaoticvolt/%s/%s/diagnostics", MQTT_CLIENT_ID, MQTT_TOPIC_AUX_DISPLAY);
        doc["stat_t"] = stateTopic;
        
        // Set value template
        char valueTemplate[64];
        sprintf(valueTemplate, "{{ value_json.%s }}", metrics[i][0]);
        doc["val_tpl"] = valueTemplate;
        
        // Set unit of measurement if provided
        if (strlen(metrics[i][2]) > 0) {
            doc["unit_of_meas"] = metrics[i][2];
        }
        
        // Set device class if provided
        if (strlen(metrics[i][3]) > 0) {
            doc["dev_cla"] = metrics[i][3];
        }
        
        // Set availability topic
        char availabilityTopic[128];
        sprintf(availabilityTopic, "chaoticvolt/%s/%s/status", MQTT_CLIENT_ID, MQTT_TOPIC_AUX_DISPLAY);
        doc["avty_t"] = availabilityTopic;
        
        // Serialize to JSON
        String payload;
        serializeJson(doc, payload);
        
        Serial.printf("[MONITOR] Publishing discovery for %s to %s\n", metrics[i][0], discoveryTopic);
        
        // Publish with retain flag
        if (!_mqttManager->publish(discoveryTopic, payload.c_str(), true)) {
            Serial.printf("[MONITOR] Failed to publish discovery for %s\n", metrics[i][0]);
            success = false;
        }
        
        // Short delay between publishes to prevent overwhelming broker
        delay(250);
    }
    
    // Update discovery time
    lastDiscoveryTime = millis();
    return success;
}