#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Display mode enumeration
enum class DisplayMode {
    TIME = 0,
    DATE = 1,
    TEMPERATURE = 2,
    HUMIDITY = 3,
    PRESSURE = 4,
    REMOTE_TEMP = 5
};

// RelayState enumeration
enum class RelayState {
    OFF = 0,
    ON = 1
};

// Command source enumeration
enum class RelayCommandSource {
    USER = 0,
    MQTT = 1,
    SYSTEM = 2
};

// Display preferences structure
struct DisplayPreferences {
    bool nightModeDimmingEnabled;
    uint8_t dayBrightness;
    uint8_t nightBrightness;
    uint8_t nightStartHour;
    uint8_t nightEndHour;
    
    // Add sensorhub credentials
    String sensorhubUsername;
    String sensorhubPassword;
    bool useSensorhub;
    
    // Constructor with default values
    DisplayPreferences() 
        : nightModeDimmingEnabled(false)
        , dayBrightness(75)
        , nightBrightness(20)
        , nightStartHour(22)
        , nightEndHour(6)
        , sensorhubUsername("")
        , sensorhubPassword("")
        , useSensorhub(false) {
    }

    // MQTT publishing settings
    bool mqttPublishEnabled;
    String mqttBrokerAddress;
    String mqttUsername;
    String mqttPassword;
    uint16_t mqttPublishInterval;
};

// Relay status structure
struct RelayStatus {
    RelayState state;
    bool override;
    
    RelayStatus() : state(RelayState::OFF), override(false) {}
    RelayStatus(RelayState s, bool o) : state(s), override(o) {}
};

// BME280 data structure
struct BME280Data {
    float temperature;
    float humidity;
    float pressure;
    
    BME280Data() : temperature(0.0f), humidity(0.0f), pressure(0.0f) {}
    BME280Data(float t, float h, float p) : temperature(t), humidity(h), pressure(p) {}
};

// Constants for BME280
constexpr float BME280_INVALID_TEMP = -999.0f;
constexpr float BME280_INVALID_HUM = -999.0f;
constexpr float BME280_INVALID_PRES = -999.0f;

// Global queue declarations
extern QueueHandle_t displayQueue;
extern QueueHandle_t sensorQueue;

// Global relay handler
class RelayControlHandler;
extern RelayControlHandler* g_relayHandler;
