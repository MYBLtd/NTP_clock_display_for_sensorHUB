#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>

// Forward declarations
class MQTTManager;

class SystemMonitor {
public:
    SystemMonitor(MQTTManager* mqttManager);
    
    void begin();
    void update();
    bool checkMemory();
    void monitorTaskStacks(TaskHandle_t* taskHandles, const char** taskNames, size_t numTasks);
    bool publishHomeAssistantDiscovery();
    
    void publishStatus(bool online = true);
    void recordNtpSyncAttempt(bool success);
    unsigned long getLastSuccessfulNtpSync() const { return _lastSuccessfulNtpSync; }
    uint32_t getNtpSyncAttempts() const { return _ntpSyncAttempts; }
    uint32_t getNtpSyncSuccesses() const { return _ntpSyncSuccesses; }
    uint32_t getNtpSyncFailures() const { return _ntpSyncFailures; }

private:
    MQTTManager* _mqttManager;
    unsigned long _lastPublishTime;
    unsigned long _startupTime;
    uint8_t _resetReason;
    uint32_t _resetCount;
    unsigned long _lastSuccessfulNtpSync;
    uint32_t _ntpSyncAttempts;
    uint32_t _ntpSyncSuccesses;
    uint32_t _ntpSyncFailures;
    unsigned long lastDiscoveryTime;
    
    // Internal methods
    void publishDiagnostics(bool retain = false);
    void publishMemoryWarning(size_t freeHeap, bool retain = false);
    void publishTaskStacks(TaskHandle_t* taskHandles, const char** taskNames, size_t numTasks, bool retain = false);
    const char* getResetReasonString() const;
    void loadResetCount();
    void saveResetCount();
    
    // Constants
    static constexpr unsigned long PUBLISH_INTERVAL = 60000;  // Publish every minute
    static constexpr size_t CRITICAL_MEMORY_THRESHOLD = 10000;  // 10KB threshold
};

#endif // SYSTEM_MONITOR_H