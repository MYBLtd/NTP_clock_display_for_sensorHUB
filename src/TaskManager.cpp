/**
 * TaskManager.cpp
 * 
 * Implementation of the TaskManager class, handling FreeRTOS task lifecycle
 * and watchdog configuration.
 */

#include "TaskManager.h"
#include "SystemDefinitions.h"
#include <ArduinoJson.h>

bool TaskManager::initializeTasks() {

    // Create display task on core 1 for consistent timing
    createTask(
        displayTask,
        "Display",
        STACK_SIZE_DISPLAY,
        PRIORITY_DISPLAY,
        &displayTaskHandle,
        1
    );
    
    // Create other tasks on core 0
    createTask(
        sensorTask,
        "SensorTask",
        STACK_SIZE_SENSOR,
        PRIORITY_SENSOR,
        &sensorTaskHandle,
        0
    );
    
    createTask(
        networkTask,
        "NetworkTask",
        STACK_SIZE_NETWORK,
        PRIORITY_NETWORK,
        &networkTaskHandle,
        0
    );
    
    createTask(
        watchdogTask,
        "WatchdogTask",
        STACK_SIZE_WATCHDOG,
        PRIORITY_WATCHDOG,
        &watchdogTaskHandle,
        0
    );

    Serial.println("Display task created");
    // Verify all tasks were created successfully
    if (!displayTaskHandle || !sensorTaskHandle || 
        !networkTaskHandle || !watchdogTaskHandle) {
        Serial.println("Failed to create one or more tasks!");
        return false;
    }
    
    // Initialize watchdog
    configureWatchdog();
    
    // Log initial task information
    monitorTaskStacks();
    
    return true;
}

void TaskManager::createTask(
    TaskFunction_t taskFunction,
    const char* taskName,
    uint32_t stackSize,
    UBaseType_t priority,
    TaskHandle_t* taskHandle,
    BaseType_t core
) {
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        taskName,
        stackSize,
        nullptr,
        priority,
        taskHandle,
        core
    );
    
    if (result != pdPASS) {
        Serial.printf("Failed to create task: %s\n", taskName);
        *taskHandle = nullptr;
    } else {
        Serial.printf("Created task: %s on core %d\n", taskName, core);
    }
}

void TaskManager::startWatchdog() {
    esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
    
    // Add tasks to watchdog
    if (displayTaskHandle) esp_task_wdt_add(displayTaskHandle);
    if (sensorTaskHandle) esp_task_wdt_add(sensorTaskHandle);
    if (networkTaskHandle) esp_task_wdt_add(networkTaskHandle);
    if (watchdogTaskHandle) esp_task_wdt_add(watchdogTaskHandle);
    
    Serial.println("Watchdog started");
}

void TaskManager::stopTasks() {
    // Remove tasks from watchdog first
    if (displayTaskHandle) esp_task_wdt_delete(displayTaskHandle);
    if (sensorTaskHandle) esp_task_wdt_delete(sensorTaskHandle);
    if (networkTaskHandle) esp_task_wdt_delete(networkTaskHandle);
    if (watchdogTaskHandle) esp_task_wdt_delete(watchdogTaskHandle);
    
    // Delete tasks
    if (displayTaskHandle) vTaskDelete(displayTaskHandle);
    if (sensorTaskHandle) vTaskDelete(sensorTaskHandle);
    if (networkTaskHandle) vTaskDelete(networkTaskHandle);
    if (watchdogTaskHandle) vTaskDelete(watchdogTaskHandle);
    
    // Clear handles
    displayTaskHandle = nullptr;
    sensorTaskHandle = nullptr;
    networkTaskHandle = nullptr;
    watchdogTaskHandle = nullptr;
    
    Serial.println("All tasks stopped");
}

void TaskManager::configureWatchdog() {
    esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
    startWatchdog();
}


void TaskManager::monitorTaskStacks(MQTTManager* mqttManager) {
    Serial.println(F("Task stack high water marks:"));
    
    if (displayTaskHandle) {
        Serial.printf_P(PSTR("Display: %u bytes\n"), 
            uxTaskGetStackHighWaterMark(displayTaskHandle));
    }
    if (sensorTaskHandle) {
        Serial.printf_P(PSTR("Sensor:  %u bytes\n"), 
            uxTaskGetStackHighWaterMark(sensorTaskHandle));
    }
    if (networkTaskHandle) {
        Serial.printf_P(PSTR("Network: %u bytes\n"), 
            uxTaskGetStackHighWaterMark(networkTaskHandle));
    }
    if (watchdogTaskHandle) {
        Serial.printf_P(PSTR("Watch:   %u bytes\n"), 
            uxTaskGetStackHighWaterMark(watchdogTaskHandle));
    }
    
    // Only publish to MQTT if mqttManager is provided and connected
    if (mqttManager && mqttManager->connected()) {
        StaticJsonDocument<512> doc;
        
        if (displayTaskHandle) {
            doc["display_stack"] = uxTaskGetStackHighWaterMark(displayTaskHandle);
        }
        if (sensorTaskHandle) {
            doc["sensor_stack"] = uxTaskGetStackHighWaterMark(sensorTaskHandle);
        }
        if (networkTaskHandle) {
            doc["network_stack"] = uxTaskGetStackHighWaterMark(networkTaskHandle);
        }
        if (watchdogTaskHandle) {
            doc["watchdog_stack"] = uxTaskGetStackHighWaterMark(watchdogTaskHandle);
        }
        
        String payload;
        serializeJson(doc, payload);
        
        mqttManager->publish("system/tasks", payload.c_str(), true);
    }
}

