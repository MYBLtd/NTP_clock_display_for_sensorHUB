#include "RelayControlHandler.h"
#include <ArduinoJson.h>

// Add these if they're not already included
#include "BabelSensor.h"
extern BabelSensor babelSensor;

RelayControlHandler* RelayControlHandler::instance = nullptr;

RelayControlHandler& RelayControlHandler::getInstance() {
    if (instance == nullptr) {
        instance = new RelayControlHandler();
    }
    return *instance;
}

RelayControlHandler::RelayControlHandler() 
    : authToken("")
    , tokenExpiry(0)
    , relayMutex(nullptr)
    , commandQueue(nullptr)
    , relayState(false)
{
    // Initialize relay states and overrides
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        currentState[i] = RelayState::OFF;
        userOverride[i] = false;
        lastStateChange[i] = 0;
    }
    
    // Create mutex for thread safety
    relayMutex = xSemaphoreCreateMutex();
    
    // Create command queue
    commandQueue = xQueueCreate(QUEUE_SIZE, sizeof(RelayCommand));
}

bool RelayControlHandler::begin() {
    // More initialization if needed
    return true;
}

void RelayControlHandler::processCommand(uint8_t relayId, RelayState state, RelayCommandSource source) {
    if (relayId >= NUM_RELAYS) {
        Serial.printf("[RELAY] Invalid relay ID: %d\n", relayId);
        return;
    }
    
    // Create a command structure
    RelayCommand cmd;
    cmd.relayId = relayId;
    cmd.state = state;
    cmd.source = source;
    cmd.timestamp = millis();
    
    // Send command to queue
    if (xQueueSend(commandQueue, &cmd, COMMAND_TIMEOUT) != pdTRUE) {
        Serial.println("[RELAY] Failed to send command to queue");
    } else {
        Serial.printf("[RELAY] Command queued: Relay %d -> %s (Source: %d)\n", 
                     relayId, 
                     state == RelayState::ON ? "ON" : "OFF",
                     static_cast<int>(source));
    }
}

RelayStatus RelayControlHandler::getRelayStatus(uint8_t relayId) const {
    if (relayId >= NUM_RELAYS) {
        return RelayStatus(RelayState::OFF, false);
    }
    
    // Take mutex to safely access state
    if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        RelayStatus status(currentState[relayId], userOverride[relayId]);
        xSemaphoreGive(relayMutex);
        return status;
    } else {
        Serial.println("[RELAY] Failed to take mutex for getRelayStatus");
        // Fall back to returning the last known state without mutex protection
        return RelayStatus(currentState[relayId], userOverride[relayId]);
    }
}

bool RelayControlHandler::authenticate() {
    // Get the token from BabelSensor if it exists
    // This is the key change - using BabelSensor's authentication
    if (babelSensor.isAuthenticated()) {
        authToken = babelSensor.getAuthToken();
        tokenExpiry = millis() + TOKEN_REFRESH_INTERVAL; // Set expiry time
        Serial.println("[RELAY] Using BabelSensor authentication token: " + authToken);
        return true;
    }
    
    // Fall back to trying login directly if BabelSensor isn't authenticated
    return refreshAuthToken();
}

bool RelayControlHandler::refreshAuthToken() {
    // Get authentication credentials from preferences to authenticate
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    if (!prefs.useSensorhub || prefs.sensorhubUsername.isEmpty() || prefs.sensorhubPassword.isEmpty()) {
        Serial.println("[RELAY] No SensorHub credentials available");
        return false;
    }
    
    // Prepare login request
    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + SENSORHUB_AUTH_ENDPOINT;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON for login
    StaticJsonDocument<256> doc;
    doc["username"] = prefs.sensorhubUsername;
    doc["password"] = prefs.sensorhubPassword;
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    // Make the request
    Serial.printf("[RELAY] Attempting login to %s\n", url.c_str());
    int httpCode = http.POST(requestBody);
    
    if (httpCode != 200) {
        Serial.printf("[RELAY] Login failed with code: %d\n", httpCode);
        http.end();
        return false;
    }
    
    // Parse response
    String response = http.getString();
    http.end();
    
    // Parse JSON response
    StaticJsonDocument<256> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (error) {
        Serial.print("[RELAY] JSON parsing error: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Get token and store it
    if (responseDoc.containsKey("token")) {
        authToken = responseDoc["token"].as<String>();
        tokenExpiry = millis() + TOKEN_REFRESH_INTERVAL;
        Serial.println("[RELAY] Authentication successful");
        return true;
    } else {
        Serial.println("[RELAY] No token found in response");
        return false;
    }
}

bool RelayControlHandler::makeAuthenticatedRequest(const char* endpoint, const char* method, const char* payload) {
    // Check if we need to refresh the token
    unsigned long now = millis();
    if (authToken.isEmpty() || (now > tokenExpiry)) {
        if (!authenticate()) {
            Serial.println("[RELAY] Failed to authenticate");
            return false;
        }
    }
    
    // Make API request with token
    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + endpoint;
    http.begin(url);
    
    // Add authentication header
    String authHeader = "Bearer " + authToken;
    http.addHeader("Authorization", authHeader);
    
    // Add content type for POST or PUT
    if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
        http.addHeader("Content-Type", "application/json");
    }
    
    // Make the request
    int httpCode = 0;
    if (strcmp(method, "GET") == 0) {
        httpCode = http.GET();
    } else if (strcmp(method, "POST") == 0) {
        httpCode = http.POST(payload);
    } else if (strcmp(method, "PUT") == 0) {
        httpCode = http.PUT(payload);
    } else if (strcmp(method, "DELETE") == 0) {
        httpCode = http.sendRequest("DELETE");
    }
    
    // Check result
    bool success = (httpCode == 200);
    if (!success) {
        Serial.printf("[RELAY] API request failed with code: %d\n", httpCode);
        Serial.printf("[RELAY] URL: %s, Method: %s\n", url.c_str(), method);
        
        // Log the error response for debugging
        String response = http.getString();
        Serial.printf("[RELAY] Error response: %s\n", response.c_str());
        
        // If unauthorized (401), clear token to force re-authentication next time
        if (httpCode == 401) {
            Serial.println("[RELAY] Authentication token invalid or expired");
            authToken = "";
        }
    }
    
    http.end();
    return success;
}

bool RelayControlHandler::setState(uint8_t relayId, RelayState newState) {
    if (relayId >= NUM_RELAYS) {
        return false;
    }
    
    // Make sure we're authenticated before making the API request
    if (authToken.isEmpty()) {
        if (!authenticate()) {
            Serial.println("[RELAY] Failed to authenticate for setState");
            return false;
        }
    }
    
    // Prepare JSON payload
    StaticJsonDocument<128> doc;
    doc["relay_id"] = relayId;
    doc["state"] = (newState == RelayState::ON) ? "ON" : "OFF";
    
    String payload;
    serializeJson(doc, payload);
    
    Serial.printf("[RELAY] Sending setState request with payload: %s\n", payload.c_str());
    
    // Make the API request
    bool success = makeAuthenticatedRequest(SENSORHUB_RELAY_ENDPOINT, "POST", payload.c_str());
    
    if (success) {
        // Update local state if API request was successful
        if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            currentState[relayId] = newState;
            lastStateChange[relayId] = millis();
            
            // Only set override if command came from user
            // (this logic should be in processCommand, but we'll update it here for safety)
            userOverride[relayId] = true;
            
            xSemaphoreGive(relayMutex);
        }
        
        // Notify state change via callback if registered
        if (stateCallback) {
            stateCallback(relayId, newState, RelayCommandSource::USER);
        }
        
        Serial.printf("[RELAY] Successfully set relay %d to %s\n", 
                     relayId, 
                     newState == RelayState::ON ? "ON" : "OFF");
        return true;
    }
    
    Serial.printf("[RELAY] Failed to set relay %d state\n", relayId);
    return false;
}

bool RelayControlHandler::setState(bool on) {
    return setState(0, on ? RelayState::ON : RelayState::OFF);
}

bool RelayControlHandler::getState() {
    return currentState[0] == RelayState::ON;
}

bool RelayControlHandler::getOverride() {
    return userOverride[0];
}

void RelayControlHandler::printRelayStatus() {
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        Serial.printf("Relay %d: %s (Override: %s)\n", 
                     i, 
                     currentState[i] == RelayState::ON ? "ON" : "OFF",
                     userOverride[i] ? "Yes" : "No");
    }
}

bool RelayControlHandler::getRelayStates(String& response) {
    // Check token and refresh if needed
    if (authToken.isEmpty()) {
        if (!authenticate()) {
            Serial.println("[RELAY] Failed to authenticate for getRelayStates");
            
            // Since we can't connect to the API, create a response with local state
            StaticJsonDocument<256> doc;
            JsonArray relays = doc.createNestedArray();
            
            for (uint8_t i = 0; i < NUM_RELAYS; i++) {
                JsonObject relay = relays.createNestedObject();
                relay["relay_id"] = i;
                relay["state"] = currentState[i] == RelayState::ON ? "ON" : "OFF";
                relay["override"] = userOverride[i];
            }
            
            serializeJson(doc, response);
            return false;
        }
    }
    
    // Make authenticated request to get relay states
    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + SENSORHUB_RELAY_ENDPOINT;
    http.begin(url);
    
    // Add authentication header
    http.addHeader("Authorization", "Bearer " + authToken);
    
    // Make the request
    int httpCode = http.GET();
    
    // Check result
    if (httpCode != 200) {
        Serial.printf("[RELAY] API request failed with code: %d\n", httpCode);
        http.end();
        
        // Fallback to local state
        StaticJsonDocument<256> doc;
        for (uint8_t i = 0; i < NUM_RELAYS; i++) {
            JsonObject relay = doc.createNestedObject(String(i));
            relay["state"] = currentState[i] == RelayState::ON ? "ON" : "OFF";
            relay["override"] = userOverride[i];
        }
        
        serializeJson(doc, response);
        return false;
    }
    
    // Get the response
    response = http.getString();
    http.end();
    return true;
}

void RelayControlHandler::handleMqttMessage(const String& topic, const String& payload) {
    // Process MQTT messages
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.print("[RELAY] JSON parsing error for MQTT: ");
        Serial.println(error.c_str());
        return;
    }
    
    if (doc.containsKey("relay_id") && doc.containsKey("state")) {
        uint8_t relayId = doc["relay_id"].as<uint8_t>();
        String stateStr = doc["state"].as<String>();
        
        if (relayId >= NUM_RELAYS) {
            Serial.printf("[RELAY] Invalid relay ID from MQTT: %d\n", relayId);
            return;
        }
        
        RelayState newState = (stateStr == "ON") ? RelayState::ON : RelayState::OFF;
        getInstance().processCommand(relayId, newState, RelayCommandSource::MQTT);
    }
}