// MQTTManager.h
#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <functional>

// Include required headers
#include "PreferencesManager.h" // Include full definition instead of forward declaration

// These constants match what was in the original implementation
#define MQTT_RECONNECT_INTERVAL 5000
#define INITIAL_RECONNECT_DELAY 500
#define MAX_RECONNECT_DELAY 60000
#define PUBLISH_RATE_LIMIT 50

// Define a callback type for message handling
using MQTTMessageHandler = std::function<void(const String&, const String&)>;

class MQTTManager {
public:
    MQTTManager();

    // Initialize - can be called with or without preferences
    bool begin(PreferencesManager& prefs);
    bool begin();  // Overload for backward compatibility
    
    // Connection management
    void loop();
    bool connect();
    bool connected();
    bool maintainConnection();  // Original method for compatibility
    void forceDisconnect();

    // Publishing methods
    bool publish(const String& topic, const String& payload);
    bool publish(const char* topic, const char* payload, bool retained = false);  // Original signature
    bool publishSensorData(const String& payload);
    bool publishRelayCommand(const String& payload);
    
    // Discovery methods
    bool publishHomeAssistantDiscovery();
    bool publishSensorDiscovery(const char* sensorType, const char* unit, const char* deviceClass);
    
    // Configuration
    void setBufferSize(uint16_t size);

    // Handle message callbacks
    void setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
    void setMessageHandler(MQTTMessageHandler handler);
    
    // Check connection status
    bool isConnectedToMQTT() const;
    
    // Utility methods
    void logState(const char* context);
    void setupSecureClient();
    
    // Subscription
    bool subscribe(const char* topic);
    void dumpConnectionDetails();
    
private:
    // WiFi Client
    WiFiClient wifiClient;
    
    // MQTT Client
    PubSubClient mqttClient;
    
    // Connection state
    bool isConnected;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    unsigned long lastPublishTime;
    unsigned long currentReconnectDelay;
    
    // Message handling
    void callback(char* topic, byte* payload, unsigned int length);
    void handleMessage(char* topic, byte* payload, unsigned int length);
    void handleCallback(char* topic, byte* payload, unsigned int length);
    
    // Configuration from preferences
    String mqttBroker;
    int mqttPort;
    String mqttClientId;
    String mqttUsername;
    String mqttPassword;
    String mqttTopicAuxDisplay;
    String mqttTopicRelay;
    
    // Message handler callback
    MQTTMessageHandler messageHandler;
    std::function<void(char*, uint8_t*, unsigned int)> userCallback;
};

#endif // MQTT_MANAGER_H