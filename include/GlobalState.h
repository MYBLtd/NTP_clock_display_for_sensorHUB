#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Forward declaration instead of including the header
class DisplayHandler;  // Add this line
#include "config.h"

class GlobalState {
public:
    static GlobalState& getInstance() {
        static GlobalState instance;
        return instance;
    }

    GlobalState(const GlobalState&) = delete;
    GlobalState& operator=(const GlobalState&) = delete;

    // Getters
    float getTemperature() const { return sensorData.temperature; }
    float getHumidity() const { return sensorData.humidity; }
    float getPressure() const { return sensorData.pressure; }
    float getRemoteTemperature() const { return sensorData.remoteTemperature; }
    bool isBMEWorking() const { return systemStatus.bmeWorking; }
    DisplayHandler* getDisplay() { return display; }
    SemaphoreHandle_t getMutex() const { return mutex; }

    // Setters
    void setBMEWorking(bool status) { systemStatus.bmeWorking = status; }
    void setMutex(SemaphoreHandle_t newMutex) { mutex = newMutex; }
    
    void updateSensorData(float temp, float hum, float pres) {
        sensorData.temperature = temp;
        sensorData.humidity = hum;
        sensorData.pressure = pres;
        sensorData.lastUpdate = millis();
    }

    void setRemoteTemperature(float temp) { 
        sensorData.remoteTemperature = temp; 
    }

    void setDisplay(DisplayHandler* newDisplay) { 
        display = newDisplay; 
    }

private:
    GlobalState() : mutex(nullptr), display(nullptr) {
        memset(&sensorData, 0, sizeof(SensorData));
        memset(&systemStatus, 0, sizeof(SystemStatus));
    }

    struct SensorData {
        float temperature;
        float humidity;
        float pressure;
        float remoteTemperature;
        uint32_t lastUpdate;
    };

    struct SystemStatus {
        bool wifiConnected;
        bool mqttConnected;
        bool bmeWorking;
        bool ntpSynced;
        uint32_t freeHeap;
        uint32_t uptime;
    };

    SensorData sensorData;
    SystemStatus systemStatus;
    SemaphoreHandle_t mutex;
    DisplayHandler* display;
};

#endif // GLOBAL_STATE_H