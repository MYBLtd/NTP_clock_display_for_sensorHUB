// MQTTManager.cpp
#include "MQTTManager.h"
#include "config.h"
#include "RelayControlHandler.h"  // Add this include for RelayState enum
#include "PreferencesManager.h"   // Add this to access PreferencesManager methods
#include <ArduinoJson.h>  // Include this for JSON handling in callbacks
#include <algorithm>      // For std::min

MQTTManager::MQTTManager() : 
    mqttClient(wifiClient),
    isConnected(false),
    lastReconnectAttempt(0),
    reconnectInterval(5000),
    lastPublishTime(0),
    currentReconnectDelay(INITIAL_RECONNECT_DELAY) {
    
    // Initialize with default values that will be overridden by preferences
    mqttBroker = MQTT_BROKER;
    mqttPort = MQTT_PORT; 
    mqttClientId = MQTT_CLIENT_ID;
    mqttUsername = MQTT_USER;
    mqttPassword = MQTT_PASSWORD;
    mqttTopicAuxDisplay = "sensor";
    mqttTopicRelay = "relay";
    
    // Configure the MQTT client with a longer socket timeout
    mqttClient.setSocketTimeout(15); // 15 seconds timeout
    mqttClient.setKeepAlive(30);     // 30 seconds keepalive
}

bool MQTTManager::begin(PreferencesManager& prefs) {
    // Set buffer size first thing
    mqttClient.setBufferSize(1024);
    
    DisplayPreferences displayPrefs = PreferencesManager::loadDisplayPreferences();
    
    // Print debug information about loaded preferences
    Serial.println("[MQTT] Loading broker settings from preferences");
    
    // Handle broker address from preferences
    if (displayPrefs.mqttBrokerAddress.length() > 0) {
        mqttBroker = displayPrefs.mqttBrokerAddress;
        Serial.printf("[MQTT] Using broker address from preferences: '%s'\n", mqttBroker.c_str());
    } else {
        mqttBroker = MQTT_BROKER;
        Serial.printf("[MQTT] Using default broker address: '%s'\n", mqttBroker.c_str());
    }
    
    // Handle credentials from preferences
    if (displayPrefs.mqttUsername.length() > 0) {
        mqttUsername = displayPrefs.mqttUsername;
        mqttPassword = displayPrefs.mqttPassword;
        Serial.printf("[MQTT] Using credentials from preferences: '%s' (len=%d)\n", 
                     mqttUsername.c_str(), mqttUsername.length());
        Serial.printf("[MQTT] Password length: %d\n", mqttPassword.length());
    } else {
        mqttUsername = MQTT_USER;
        mqttPassword = MQTT_PASSWORD;
        Serial.printf("[MQTT] Using default credentials: '%s'\n", mqttUsername.c_str());
    }
    
    // DEBUG: Verify final broker settings before creating MQTT client
    Serial.println("[MQTT] FINAL BROKER SETTINGS:");
    Serial.printf("[MQTT] Broker: '%s'\n", mqttBroker.c_str());
    Serial.printf("[MQTT] Port: %d\n", mqttPort);
    Serial.printf("[MQTT] Username: '%s'\n", mqttUsername.c_str());
    Serial.printf("[MQTT] Client ID: '%s'\n", MQTT_CLIENT_ID);
    
    // Set up the MQTT client 
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);
    
    // Add additional debugging
    Serial.printf("[MQTT] MQTT client server address set to: %s:%d\n", mqttBroker.c_str(), mqttPort);
    
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
    
    return connect();
}

bool MQTTManager::begin() {
    // First, load preferences directly
    DisplayPreferences displayPrefs = PreferencesManager::loadDisplayPreferences();
    
    // Debug output for the preferences
    Serial.println("[MQTT] Loading broker settings from preferences:");
    
    // Use address from preferences if available
    if (displayPrefs.mqttBrokerAddress.length() > 0) {
        mqttBroker = displayPrefs.mqttBrokerAddress;
        Serial.printf("[MQTT] Using broker address from preferences: '%s'\n", mqttBroker.c_str());
    } else {
        mqttBroker = MQTT_BROKER;
        Serial.printf("[MQTT] Using default broker address: '%s'\n", mqttBroker.c_str());
    }
    
    // Load credentials
    if (displayPrefs.mqttUsername.length() > 0) {
        mqttUsername = displayPrefs.mqttUsername;
        mqttPassword = displayPrefs.mqttPassword;
        Serial.printf("[MQTT] Using credentials from preferences for user: '%s'\n", mqttUsername.c_str());
    } else {
        mqttUsername = MQTT_USER;
        mqttPassword = MQTT_PASSWORD;
        Serial.printf("[MQTT] Using default credentials for user: '%s'\n", mqttUsername.c_str());
    }
    
    // Configure settings based on preferences
    mqttPort = MQTT_PORT;
    mqttClientId = MQTT_CLIENT_ID;
    mqttTopicAuxDisplay = "sensor";
    mqttTopicRelay = "relay";
    
    // Now set up the MQTT client with these values
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
    
    return connect();
}

void MQTTManager::loop() {
    // Get current time
    unsigned long now = millis();
    
    // Check if still connected
    if (!mqttClient.connected()) {
        if (isConnected) {
            // We thought we were connected but we're not
            Serial.println("[MQTT] Connection lost");
            isConnected = false;
            
            // Force disconnect to clean up resources
            forceDisconnect();
        }
        
        // Try to reconnect with exponential backoff
        if (now - lastReconnectAttempt > currentReconnectDelay) {
            lastReconnectAttempt = now;
            
            Serial.printf("[MQTT] Attempting reconnection (backoff: %lu ms)\n", currentReconnectDelay);
            
            // Make sure previous connection is fully closed
            mqttClient.disconnect();
            wifiClient.stop();
            delay(100);
            
            if (connect()) {
                Serial.println("[MQTT] Reconnection successful");
                lastReconnectAttempt = 0;
                currentReconnectDelay = INITIAL_RECONNECT_DELAY;
            } else {
                // Increase backoff time
                currentReconnectDelay = std::min(currentReconnectDelay * 2, (unsigned long)MAX_RECONNECT_DELAY);
                Serial.printf("[MQTT] Reconnection failed, next attempt in %lu ms\n", currentReconnectDelay);
                
                // Debug current settings
                Serial.printf("[MQTT] Current settings - Broker: %s, Port: %d, User: %s\n", 
                              mqttBroker.c_str(), mqttPort, mqttUsername.c_str());
            }
        } else {
            // Add debugging for inactive periods
            static unsigned long lastDebugOutput = 0;
            if (now - lastDebugOutput > 10000) { // Every 10 seconds
                Serial.printf("[MQTT] Waiting %lu ms before next reconnect attempt\n", 
                             currentReconnectDelay - (now - lastReconnectAttempt));
                lastDebugOutput = now;
            }
        }
    } else {
        // Client connected - process incoming messages
        mqttClient.loop();
        
        // Check for periodic status updates
        static unsigned long lastStatusUpdate = 0;
        if (now - lastStatusUpdate > 300000) { // Every 5 minutes
            String statusTopic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_AUX_DISPLAY) + "/status";
            mqttClient.publish(statusTopic.c_str(), "online", true);
            lastStatusUpdate = now;
            Serial.println("[MQTT] Published periodic status update");
        }
    }
}

bool MQTTManager::maintainConnection() {
    unsigned long now = millis();
    static unsigned long lastConnectionCheck = 0;
    
    // Force a connection check every 30 seconds regardless of state
    if (now - lastConnectionCheck >= 30000) {
        lastConnectionCheck = now;
        Serial.println("[MQTT] Periodic connection check");
        
        if (!mqttClient.connected()) {
            Serial.println("[MQTT] Not connected during periodic check");
            // Explicitly disconnect to ensure clean state
            mqttClient.disconnect();
            wifiClient.stop();
            delay(100);
            return connect();
        } else {
            // Connected - just return true
            return true;
        }
    }
    
    // Regular connection maintenance logic
    if (!mqttClient.connected()) {
        // Log current time remaining until reconnect
        Serial.printf("[MQTT] Not connected, %lu ms until next attempt\n", 
                    (now - lastReconnectAttempt >= currentReconnectDelay) ? 
                    0 : (currentReconnectDelay - (now - lastReconnectAttempt)));
        
        if (now - lastReconnectAttempt >= currentReconnectDelay) {
            Serial.println("[MQTT] Connection lost, cleaning up before reconnect");
            
            // Ensure complete disconnection
            forceDisconnect();
            Serial.println("[MQTT] Previous connection resources released");
            
            lastReconnectAttempt = now;
            
            if (connect()) {
                Serial.println("[MQTT] Reconnection successful");
                currentReconnectDelay = INITIAL_RECONNECT_DELAY;
                return true;
            } else {
                // Limit maximum backoff to 30 seconds
                currentReconnectDelay = std::min(currentReconnectDelay * 2, (unsigned long)30000);
                Serial.printf("[MQTT] Reconnection failed, next attempt in %lu ms\n", currentReconnectDelay);
                return false;
            }
        }
        return false;
    }
    
    // If connected, process incoming messages
    mqttClient.loop();
    return true;
}

bool MQTTManager::connected() {
    return mqttClient.connected();
}

bool MQTTManager::publish(const String& topic, const String& payload) {
    if (!isConnectedToMQTT()) {
        return false;
    }
    
    return mqttClient.publish(topic.c_str(), payload.c_str());
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retained) {
    if (!connected()) {
        Serial.println("MQTT: Cannot publish - not connected");
        return false;
    }

    // Get publish interval from preferences
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    unsigned long publishInterval = prefs.mqttPublishEnabled ? 
                                    prefs.mqttPublishInterval * 1000 : // Convert to ms
                                    PUBLISH_RATE_LIMIT;
    
    // Rate limit publishing to respect preferences and prevent broker overload
    unsigned long now = millis();
    if (now - lastPublishTime < publishInterval) {
        unsigned long waitTime = publishInterval - (now - lastPublishTime);
        if (waitTime > 50) { // Only delay if significant time remains
            Serial.printf("MQTT: Rate limiting publish, waiting %lu ms\n", waitTime);
            delay(50); // Small delay to prevent busy-waiting
        }
    }

    const int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            delay((1 << retry) * 200); // Exponential backoff for retries
            Serial.printf("MQTT: Retry %d/%d publishing to %s\n", retry+1, maxRetries, topic);
        }

        if (mqttClient.publish(topic, payload, retained)) {
            lastPublishTime = millis();
            return true;
        }
        
        Serial.printf("MQTT: Publish attempt %d failed for topic: %s\n", retry + 1, topic);
    }
    
    return false;
}

bool MQTTManager::publishSensorData(const String& payload) {
    return publish(mqttTopicAuxDisplay, payload);
}

bool MQTTManager::publishRelayCommand(const String& payload) {
    return publish(mqttTopicRelay, payload);
}

void MQTTManager::forceDisconnect() {
    // Properly clean up any existing connection
    if (mqttClient.connected()) {
        // Try to publish offline status before disconnecting
        String uniqueClientId = MQTT_CLIENT_ID;
        uniqueClientId.replace(":", "");
        String statusTopic = String("chaoticvolt/") + uniqueClientId + "/sensor/status";
        
        mqttClient.publish(statusTopic.c_str(), "offline", true);
        delay(10);  // Brief delay to allow message to be sent
        
        mqttClient.disconnect();
    }
    
    // Always stop the WiFi client to ensure socket is closed
    wifiClient.stop();
    
    // Reset state variables
    isConnected = false;
    lastReconnectAttempt = 0;
    currentReconnectDelay = INITIAL_RECONNECT_DELAY;
}

bool MQTTManager::connect() {
    // Reset watchdog to avoid timeouts during connection process
    esp_task_wdt_reset();
    
    // Ensure any existing connections are fully closed
    mqttClient.disconnect();
    delay(50);
    wifiClient.stop();
    delay(100);
    
    // Use the consistent client ID from configuration
    String uniqueClientId = MQTT_CLIENT_ID;
    
    Serial.printf("[MQTT] Connecting to broker %s:%d with client ID: %s\n", 
                 mqttBroker.c_str(), mqttPort, uniqueClientId.c_str());
    
    // First test raw TCP connection to verify network path is clear
    Serial.printf("[MQTT] Testing TCP connection to %s:%d...\n", mqttBroker.c_str(), mqttPort);
    WiFiClient testClient;
    testClient.setTimeout(5000); // 5 second timeout for connection test
    
    if (!testClient.connect(mqttBroker.c_str(), mqttPort)) {
        Serial.println("[MQTT] TCP connection test failed - basic connectivity issue");
        Serial.printf("[MQTT] Connection error: %d\n", testClient.getWriteError());
        testClient.stop();
        return false;
    }
    
    Serial.println("[MQTT] TCP connection test successful");
    testClient.stop();
    delay(100);
    
    // Configure longer timeout for the WiFi client
    wifiClient.setTimeout(15000);  // 15 seconds timeout
    
    // Set up Last Will and Testament for clean disconnection detection
    String statusTopic = String("chaoticvolt/") + uniqueClientId + "/sensor/status";
    
    // Reset watchdog again before actual MQTT connection
    esp_task_wdt_reset();
    
    // Connect with credentials if available
    bool result = false;
    
    if (mqttUsername.length() > 0) {
        Serial.printf("[MQTT] Connecting with credentials: %s\n", mqttUsername.c_str());
        Serial.printf("[MQTT] Password length: %d characters\n", mqttPassword.length());
        
        result = mqttClient.connect(
            uniqueClientId.c_str(),
            mqttUsername.c_str(), 
            mqttPassword.c_str(),
            statusTopic.c_str(),  // LWT topic
            0,                    // QoS
            true,                 // Retain
            "offline"             // LWT message
        );
    } else {
        Serial.println("[MQTT] Connecting without credentials");
        result = mqttClient.connect(uniqueClientId.c_str());
    }
    
    // Reset watchdog after connection attempt
    esp_task_wdt_reset();
    
    if (result) {
        Serial.println("[MQTT] Connected successfully!");
        isConnected = true;
        
        // Immediate publish of online status with retain flag
        if (!mqttClient.publish(statusTopic.c_str(), "online", true)) {
            Serial.println("[MQTT] Failed to publish initial status message");
        } else {
            Serial.println("[MQTT] Published online status");
        }
        
        // Subscribe to needed topics
        String relayTopic = String("chaoticvolt/") + uniqueClientId + "/relay/command";
        if (!mqttClient.subscribe(relayTopic.c_str())) {
            Serial.println("[MQTT] Failed to subscribe to relay topic");
        } else {
            Serial.printf("[MQTT] Subscribed to topic: %s\n", relayTopic.c_str());
        }
        
        // Reset reconnection parameters on successful connection
        currentReconnectDelay = INITIAL_RECONNECT_DELAY;
    } else {
        int state = mqttClient.state();
        Serial.printf("[MQTT] Connection failed, state: %d\n", state);
        
        // Detailed error reporting
        switch(state) {
            case -4: Serial.println("[MQTT] Connection timeout"); break;
            case -3: Serial.println("[MQTT] Connection lost"); break;
            case -2: Serial.println("[MQTT] Connection failed"); break;
            case -1: Serial.println("[MQTT] Client disconnected"); break;
            case 1: Serial.println("[MQTT] Bad protocol version"); break;
            case 2: Serial.println("[MQTT] Bad client ID"); break;
            case 3: Serial.println("[MQTT] Server unavailable"); break;
            case 4: Serial.println("[MQTT] Bad username/password"); break;
            case 5: Serial.println("[MQTT] Not authorized"); break;
            default: Serial.println("[MQTT] Unknown error"); break;
        }
        
        // Additional diagnostics
        Serial.printf("[MQTT] Using broker: %s:%d\n", mqttBroker.c_str(), mqttPort);
        Serial.printf("[MQTT] Username: %s (length: %d)\n", mqttUsername.c_str(), mqttUsername.length());
        Serial.printf("[MQTT] Password length: %d\n", mqttPassword.length());
        Serial.printf("[MQTT] Client ID: %s\n", uniqueClientId.c_str());
        
        isConnected = false;
    }
    
    return isConnected;
}


bool MQTTManager::subscribe(const char* topic) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT: Cannot subscribe - not connected");
        return false;
    }
    return mqttClient.subscribe(topic, 1);
}

void MQTTManager::setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback) {
    userCallback = callback;
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->handleCallback(topic, payload, length);
    });
}

void MQTTManager::setupSecureClient() {
    // This is now a no-op since we're using plain WiFiClient
    // But left for compatibility with existing code
}

void MQTTManager::logState(const char* context) {
    const char* stateStr;
    switch(mqttClient.state()) {
        case -4: stateStr = "TIMEOUT"; break;
        case -3: stateStr = "LOST"; break;
        case -2: stateStr = "FAILED"; break;
        case -1: stateStr = "DISCONNECTED"; break;
        case 0: stateStr = "CONNECTED"; break;
        case 1: stateStr = "BAD_PROTOCOL"; break;
        case 2: stateStr = "BAD_CLIENT_ID"; break;
        case 3: stateStr = "UNAVAILABLE"; break;
        case 4: stateStr = "BAD_CREDENTIALS"; break;
        case 5: stateStr = "UNAUTHORIZED"; break;
        default: stateStr = "UNKNOWN"; break;
    }
    Serial.printf("MQTT State [%s]: %s (%d)\n", context, stateStr, mqttClient.state());
}

// Callback method that routes to either handleMessage or the user callback
void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string for easier handling
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);
    
    // Process messages
    if (String(topic) == mqttTopicRelay) {
        handleMessage(topic, payload, length);
    }
    
    // Call any registered handler
    if (messageHandler) {
        messageHandler(String(topic), message);
    }
}

// Generic message handler
void MQTTManager::handleMessage(char* topic, byte* payload, unsigned int length) {
    String message((char*)payload, length);
    
    // Parse JSON message
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.print(F("JSON parsing failed: "));
        Serial.println(error.c_str());
        return;
    }
    
    // Check if we have the required fields
    if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
        Serial.println(F("Missing required fields in MQTT message"));
        return;
    }
    
    uint8_t relayId = doc["relay_id"].as<uint8_t>();
    const char* stateStr = doc["state"].as<const char*>();
    
    RelayState state = (strcmp(stateStr, "ON") == 0) ? RelayState::ON : RelayState::OFF;
    
    // Forward command to relay handler if available
    extern RelayControlHandler* g_relayHandler;
    if (g_relayHandler) {
        g_relayHandler->processCommand(relayId, state, RelayCommandSource::MQTT);
    }
}

// User callback wrapper
void MQTTManager::handleCallback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    String payloadStr;
    payloadStr.reserve(length);
    for(unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    // Debug logging
    Serial.printf("MQTT: Received message on topic: %s\n", topic);
    
    // Check if this is a relay command
    String relayCommandTopic = String("chaoticvolt/") + String(MQTT_CLIENT_ID) + "/" + String(MQTT_TOPIC_RELAY) + "/command";
    if (String(topic) == relayCommandTopic) {
        Serial.println("MQTT: Received relay command");
        
        // Handle relay commands
        extern RelayControlHandler* g_relayHandler;
        if (g_relayHandler) {
            RelayControlHandler::handleMqttMessage(String(topic), payloadStr);
        }
    }
    
    // Call user callback if set
    if (userCallback) {
        userCallback(topic, (uint8_t*)payload, length);
    }
}

void MQTTManager::setMessageHandler(MQTTMessageHandler handler) {
    messageHandler = handler;
}

bool MQTTManager::isConnectedToMQTT() const {
    return isConnected;
}

void MQTTManager::setBufferSize(uint16_t size) {
    mqttClient.setBufferSize(size);
}

bool MQTTManager::publishHomeAssistantDiscovery() {
    Serial.println("Publishing Home Assistant discovery with consistent device identification");
    
    // Increase the MQTT buffer size
    setBufferSize(1024);
    
    // Publish discovery information for temperature and humidity - one at a time with delay
    bool tempSuccess = publishSensorDiscovery("temperature", "°C", "temperature");
    delay(500); // Add delay between publications to avoid stack issues
    
    bool humSuccess = publishSensorDiscovery("humidity", "%", "humidity");
    
    return tempSuccess && humSuccess;
}

bool MQTTManager::publishSensorDiscovery(const char* sensorType, const char* unit, const char* deviceClass) {
    if (!connected()) {
        Serial.println("MQTT: Cannot publish discovery - not connected");
        return false;
    }
    
    // First, set a larger buffer size for the MQTT client to handle larger payloads
    setBufferSize(1024);
    
    // Create a unique ID with a version suffix to force fresh entity creation
    char uniqueId[64];
    sprintf(uniqueId, "%s_%s_v3", MQTT_CLIENT_ID, sensorType);
    
    // Create the discovery topic
    char discoveryTopic[128];
    sprintf(discoveryTopic, "chaoticvolt/sensorhub1/sensor/%s/config", uniqueId);
    
    // Create a friendly display name
    char displayName[64];
    sprintf(displayName, "%s %s", MQTT_CLIENT_ID, sensorType);
    
    // Construct all components of the payload directly using a StaticJsonDocument
    StaticJsonDocument<512> doc;
    
    // First set up device information
    JsonObject device = doc.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(MQTT_CLIENT_ID);  // Use MQTT_CLIENT_ID directly
    
    device["name"] = MQTT_CLIENT_ID;
    device["mdl"] = FIRMWARE_VERSION;
    device["mf"] = "chaoticvolt";
    
    // Set required discovery fields directly
    doc["name"] = displayName;
    doc["uniq_id"] = uniqueId;
    if (strlen(deviceClass) > 0) {
        doc["dev_cla"] = deviceClass;
    }
    
    // Construct state topic directly with sprintf
    char stateTopic[128];
    sprintf(stateTopic, "chaoticvolt/%s/%s/sensors", MQTT_CLIENT_ID, MQTT_TOPIC_AUX_DISPLAY);
    doc["stat_t"] = stateTopic;
    
    // Set the value template
    char valueTemplate[64];
    sprintf(valueTemplate, "{{ value_json.%s }}", sensorType);
    doc["val_tpl"] = valueTemplate;
    
    // Set unit of measurement if provided
    if (strlen(unit) > 0) {
        doc["unit_of_meas"] = unit;
    }
    
    // CRITICAL: Set availability topic directly
    char availabilityTopic[128];
    sprintf(availabilityTopic, "chaoticvolt/%s/%s/status", MQTT_CLIENT_ID, MQTT_TOPIC_AUX_DISPLAY);
    doc["avty_t"] = availabilityTopic;
    
    // Serialize to a char array, not a String
    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    
    // Print the exact payload for debugging
    Serial.println("=== SENSOR DISCOVERY PAYLOAD ===");
    Serial.println(payload);
    Serial.println("===============================");
    
    // First publish the status as "online" to ensure entity is available
    publish(availabilityTopic, "online", true);
    delay(100); // Short delay
    
    // Then publish the discovery configuration
    bool success = publish(discoveryTopic, payload, true);
    
    if (success) {
        Serial.printf("Successfully published discovery for %s sensor\n", sensorType);
    } else {
        Serial.printf("Failed to publish discovery for %s sensor\n", sensorType);
    }
    
    delay(200); // Allow time for broker processing
    
    return success;
}

void MQTTManager::dumpConnectionDetails() {
    Serial.println("======= MQTT CONNECTION DETAILS =======");
    Serial.printf("Broker: %s:%d\n", mqttBroker.c_str(), mqttPort);
    Serial.printf("Username: '%s' (length=%d)\n", mqttUsername.c_str(), mqttUsername.length());
    Serial.printf("Password: (length=%d)\n", mqttPassword.length()); 
    Serial.printf("Client ID: '%s'\n", MQTT_CLIENT_ID);
    Serial.printf("Last reconnect attempt: %lu ms ago\n", millis() - lastReconnectAttempt);
    Serial.printf("Current reconnect delay: %lu ms\n", currentReconnectDelay);
    Serial.printf("Connected state: %s\n", mqttClient.connected() ? "YES" : "NO");
    Serial.printf("Internal state tracker: %s\n", isConnected ? "Connected" : "Disconnected");
    
    // Test direct TCP connection
    WiFiClient testClient;
    testClient.setTimeout(5000);
    bool tcpTest = testClient.connect(mqttBroker.c_str(), mqttPort);
    Serial.printf("TCP connection test: %s\n", tcpTest ? "SUCCESS" : "FAILED");
    if (!tcpTest) {
        Serial.printf("TCP error: %d\n", testClient.getWriteError());
    }
    testClient.stop();
    
    Serial.println("=======================================");
}
