#include "BabelSensor.h"
#include "config.h"
#include <algorithm> // For std::min

BabelSensor::BabelSensor(const char* url) 
    : serverUrl(url), lastUpdate(0), lastTemperature(0.0), 
      enabled(false), lastTokenRefresh(0) {
    Serial.println("[BABEL] Initialized with URL: " + String(url));
}

bool BabelSensor::init() {
    Serial.println("[BABEL] Initialization started");
    
    // Load preferences to check if sensorhub is enabled
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    // Set enabled flag according to preferences
    enabled = prefs.useSensorhub;
    
    if (!enabled) {
        Serial.println("[BABEL] Sensorhub disabled in preferences");
        return false;
    }
    
    Serial.println("[BABEL] Sensorhub enabled in preferences");
    
    // Test connection
    HTTPClient http;
    String testUrl = "http://" + String(serverUrl);
    
    Serial.println("[BABEL] Testing connection to: " + testUrl);
    http.begin(testUrl);
    
    int httpCode = http.GET();
    Serial.printf("[BABEL] Test connection result: %d\n", httpCode);
    
    http.end();
    
    // Only proceed if we have a response from the server
    if (httpCode > 0) {
        // Try to login with stored credentials
        if (loginWithStoredCredentials()) {
            Serial.println("[BABEL] Successfully authenticated with stored credentials");
            return true;
        } else {
            Serial.println("[BABEL] Failed to authenticate with stored credentials");
            return false;
        }
    }
    
    return false; // Connection failed
}

bool BabelSensor::loginWithStoredCredentials() {
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    if (!prefs.useSensorhub || prefs.sensorhubUsername.isEmpty()) {
        Serial.println("[BABEL] No stored credentials or sensorhub disabled");
        return false;
    }
    
    Serial.printf("[BABEL] Attempting login with stored credentials for user: %s\n", 
                 prefs.sensorhubUsername.c_str());
    
    return login(prefs.sensorhubUsername.c_str(), prefs.sensorhubPassword.c_str());
}

bool BabelSensor::updateCredentials(const String& username, const String& password) {
    // Update credentials and attempt to login
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    prefs.sensorhubUsername = username;
    prefs.sensorhubPassword = password;
    prefs.useSensorhub = true;
    
    // Save the updated preferences
    PreferencesManager::saveDisplayPreferences(prefs);
    
    // Try to login with the new credentials
    return login(username.c_str(), password.c_str());
}

void BabelSensor::setEnabled(bool enableState) {
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    if (prefs.useSensorhub != enableState) {
        prefs.useSensorhub = enableState;
        PreferencesManager::saveDisplayPreferences(prefs);
    }
    
    enabled = enableState;
}

bool BabelSensor::isEnabled() const {
    return enabled;
}

bool BabelSensor::ensureAuthenticated() {
    // If not authenticated or token is old, try to authenticate
    unsigned long now = millis();
    
    // Use TOKEN_REFRESH_INTERVAL_MS instead of TOKEN_REFRESH_INTERVAL
    if (authToken.isEmpty() || (now - lastTokenRefresh >= TOKEN_REFRESH_INTERVAL_MS)) {
        return loginWithStoredCredentials();
    }
    
    return !authToken.isEmpty();
}

float BabelSensor::getRemoteTemperature() {
    unsigned long now = millis();
    
    // Check if sensorhub is enabled
    if (!enabled) {
        Serial.println("[BABEL] SensorHub is disabled in preferences");
        DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
        if (prefs.useSensorhub) {
            Serial.println("[BABEL] WARNING: Preferences show SensorHub should be enabled!");
            Serial.println("[BABEL] Attempting to re-enable...");
            enabled = true;
            // Still return 0 this time, but next call should work
        }
        return 0.0f; // Return 0 if disabled
    }
    
    // Use cached value if not time to update yet
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return lastTemperature;
    }
    
    Serial.println("[BABEL] Getting remote temperature...");
    
    // Check authentication
    if (!ensureAuthenticated()) {
        Serial.println("[BABEL] Authentication failed, returning last temperature: " + String(lastTemperature));
        
        // Force authentication attempt on next call
        authToken = "";
        lastTokenRefresh = 0;
        
        return lastTemperature;
    }
    
    HTTPClient http;
    String sensorUrl = "http://" + serverUrl + String(API_SENSORS_ENDPOINT);
    
    Serial.println("[BABEL] Requesting sensors from: " + sensorUrl);
    http.begin(sensorUrl);
    http.addHeader("Authorization", "Bearer " + authToken);
    
    int httpCode = http.GET();
    Serial.printf("[BABEL] Sensor API response code: %d\n", httpCode);
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.println("[BABEL] Response received, length: " + String(response.length()));
        
        // Log the first part of the response for debugging
        if (response.length() > 0) {
            // Use std::min with explicit type to avoid the error
            String responseSample = response.substring(0, std::min<size_t>(200, response.length()));
            Serial.println("[BABEL] Response sample: " + responseSample);
        }
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            Serial.println("[BABEL] JSON parsing successful");
            
            // Check if response is an array
            if (doc.is<JsonArray>()) {
                bool babelSensorFound = false;
                JsonArray array = doc.as<JsonArray>();
                
                Serial.printf("[BABEL] Processing array with %d elements\n", array.size());
                
                for (JsonVariant sensor : array) {
                    // Dump sensor info for debugging
                    Serial.println("[BABEL] Sensor found: ");
                    String keys = "";
                    JsonObject obj = sensor.as<JsonObject>();
                    for (JsonPair kv : obj) {
                        keys += String(kv.key().c_str()) + ", ";
                    }
                    Serial.println("[BABEL] Keys in sensor: " + keys);
                    
                    // Check for different possible keys that might indicate this is a BabelSensor
                    if (sensor.containsKey("isBabelSensor") && sensor["isBabelSensor"].as<bool>()) {
                        babelSensorFound = true;
                        Serial.println("[BABEL] Found via isBabelSensor flag");
                        if (sensor.containsKey("babelTemperature")) {
                            float newTemp = sensor["babelTemperature"].as<float>();
                            Serial.printf("[BABEL] Found temperature: %.2f\n", newTemp);
                            lastTemperature = newTemp;
                            lastUpdate = now;
                        } else {
                            Serial.println("[BABEL] No babelTemperature field found");
                        }
                        break;
                    } 
                    // Try alternative field name
                    else if (sensor.containsKey("type") && sensor["type"].as<String>() == "babel") {
                        babelSensorFound = true;
                        Serial.println("[BABEL] Found via type=babel");
                        if (sensor.containsKey("temperature")) {
                            float newTemp = sensor["temperature"].as<float>();
                            Serial.printf("[BABEL] Found temperature: %.2f\n", newTemp);
                            lastTemperature = newTemp;
                            lastUpdate = now;
                        } else {
                            Serial.println("[BABEL] No temperature field found");
                        }
                        break;
                    }
                    // Try more generic approach if we can identify the right sensor
                    else if (sensor.containsKey("name") && 
                            (sensor["name"].as<String>().indexOf("babel") >= 0 ||
                             sensor["name"].as<String>().indexOf("remote") >= 0)) {
                        babelSensorFound = true;
                        Serial.println("[BABEL] Found via name containing 'babel' or 'remote'");
                        if (sensor.containsKey("temperature") || sensor.containsKey("value")) {
                            float newTemp = sensor.containsKey("temperature") ? 
                                sensor["temperature"].as<float>() : sensor["value"].as<float>();
                            Serial.printf("[BABEL] Found temperature: %.2f\n", newTemp);
                            lastTemperature = newTemp;
                            lastUpdate = now;
                        } else {
                            Serial.println("[BABEL] No temperature field found");
                        }
                        break;
                    }
                }
                
                if (!babelSensorFound) {
                    Serial.println("[BABEL] No suitable sensor found in response");
                }
            } else {
                Serial.println("[BABEL] Response is not an array, checking for direct temperature value");
                
                // Try direct object with temperature
                if (doc.containsKey("temperature")) {
                    float newTemp = doc["temperature"].as<float>();
                    Serial.printf("[BABEL] Found direct temperature: %.2f\n", newTemp);
                    lastTemperature = newTemp;
                    lastUpdate = now;
                } else {
                    Serial.println("[BABEL] No direct temperature field found");
                }
            }
        } else {
            Serial.printf("[BABEL] JSON parsing failed: %s\n", error.c_str());
        }
    } else if (httpCode == 401) {
        Serial.println("[BABEL] Authentication failed (401), clearing token");
        authToken = ""; // Clear the token so we'll try to authenticate again next time
    } else {
        Serial.printf("[BABEL] HTTP request failed, code: %d\n", httpCode);
    }
    
    http.end();
    Serial.printf("[BABEL] Returning temperature: %.2f\n", lastTemperature);
    return lastTemperature;
}

bool BabelSensor::login(const char* username, const char* password) {
// In the login method:
    HTTPClient http;
    String loginUrl = "http://" + serverUrl + String(API_LOGIN_ENDPOINT);
    
    Serial.println("[BABEL] Attempting login at: " + loginUrl);
    Serial.printf("[BABEL] Using credentials: %s / %s\n", username, "********");
    
    http.begin(loginUrl);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<200> credentials;
    credentials["username"] = username;
    credentials["password"] = password;
    
    String requestBody;
    serializeJson(credentials, requestBody);
    Serial.println("[BABEL] Login request body: " + requestBody);
    
    int httpCode = http.POST(requestBody);
    Serial.printf("[BABEL] Login HTTP response code: %d\n", httpCode);
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.println("[BABEL] Login response: " + response);
        
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            // Log all keys in the response for debugging
            String keys = "";
            JsonObject obj = doc.as<JsonObject>();
            for (JsonPair kv : obj) {
                keys += String(kv.key().c_str()) + ", ";
            }
            Serial.println("[BABEL] Keys in login response: " + keys);
            
            // Try different possible token field names
            if (doc.containsKey("token")) {
                authToken = doc["token"].as<String>();
                Serial.println("[BABEL] Authentication successful, token received");
                lastTokenRefresh = millis();
                http.end();
                return true;
            } else if (doc.containsKey("access_token")) {
                authToken = doc["access_token"].as<String>();
                Serial.println("[BABEL] Authentication successful, access_token received");
                lastTokenRefresh = millis();
                http.end();
                return true;
            } else if (doc.containsKey("jwt") || doc.containsKey("JWT")) {
                authToken = doc.containsKey("jwt") ? doc["jwt"].as<String>() : doc["JWT"].as<String>();
                Serial.println("[BABEL] Authentication successful, JWT received");
                lastTokenRefresh = millis();
                http.end();
                return true;
            } else {
                Serial.println("[BABEL] No token found in response");
            }
        } else {
            Serial.printf("[BABEL] JSON parsing failed: %s\n", error.c_str());
        }
    } else {
        Serial.printf("[BABEL] Authentication failed with code: %d\n", httpCode);
    }
    
    http.end();
    return false;
}