#include "WebHandlers.h"
#include "DisplayHandler.h"
#include "icons.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include <ArduinoJson.h>
#include "GlobalState.h"
#include "PreferencesManager.h"
#include "RelayControlHandler.h"
#include <base64.h>
#include "BabelSensor.h"
#include "WiFiConnectionManager.h"

extern BabelSensor babelSensor;
extern GlobalState* g_state;
extern PreferencesManager prefsManager;
static String cachedPreferencesJson;
static unsigned long lastPreferencesCacheTime = 0;
static const unsigned long PREFERENCES_CACHE_DURATION = 10000; // 10 seconds

void handleWiFiStatus();
void handleWiFiReconnect();
void handleSetWifiCredentials();

void addCorsHeaders(WebServer* server) {
    if (!server) return;
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    // Add cache control headers
    server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server->sendHeader("Pragma", "no-cache");
    server->sendHeader("Expires", "-1");

    // Serve the appropriate page based on the connection state
    if (webManager.isInAPMode()) {
        server->send(200, "text/html", SETUP_PAGE_HTML);
    } else {
        server->send(200, "text/html", PREFERENCES_PAGE_HTML);
    }
}

String getNetworksJson() {
    String json = "[";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + 
                String(WiFi.RSSI(i)) + ",\"encrypted\":" + 
                String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    return json;
}

void handleConnect() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    String ssid = server->arg("ssid");
    String password = server->arg("password");
    
    if (ssid.isEmpty()) {
        server->send(400, "text/plain", "SSID is required");
        return;
    }

    Serial.printf("[WEB] Attempting to connect to SSID: %s\n", ssid.c_str());
    
    // Create response page with appropriate timeout
    String html = "<html><head><meta http-equiv='refresh' content='";
    html += "15;url=/'><title>WiFi Connection</title></head>";
    html += "<body><h1>Connecting to WiFi Network</h1>";
    html += "<p>The device is now trying to connect to: <strong>" + ssid + "</strong></p>";
    html += "<p>If connection is successful, the device will restart in normal mode.</p>";
    html += "<p>Please wait about 15 seconds...</p>";
    html += "<p>If you cannot connect after 30 seconds, the device will return to setup mode.</p>";
    html += "</body></html>";
    
    // Send response first, then try to connect
    server->send(200, "text/html", html);
    server->client().stop(); // Ensure the response is sent
    delay(500);  // Short delay to ensure response is sent
    
    // Use the WiFiConnectionManager to manage connection
    auto& wifiManager = WiFiConnectionManager::getInstance();
    
    // Store credentials and attempt connection
    if (wifiManager.connect(ssid, password)) {
        Serial.println("[WEB] WiFi connection successful, restarting...");
        delay(2000); // Give time for client to receive redirect
        ESP.restart(); // Restart to apply new configuration cleanly
    } else {
        Serial.println("[WEB] WiFi connection failed, returning to portal mode");
        // Stay in portal mode - user will be redirected back to setup page
    }
}

void handleGetPreferences() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);
    
    // Add cache headers to improve browser performance
    server->sendHeader("Cache-Control", "max-age=10");
    
    // Use cached JSON if available and not expired
    unsigned long now = millis();
    if (cachedPreferencesJson.length() > 0 && (now - lastPreferencesCacheTime < PREFERENCES_CACHE_DURATION)) {
        server->send(200, "application/json", cachedPreferencesJson);
        return;
    }
    
    // Get preferences from cached manager (fast)
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    // Prepare JSON response
    StaticJsonDocument<1024> doc; // Increased size for MQTT settings
    doc["success"] = true;
    
    JsonObject data = doc.createNestedObject("data");
    data["nightDimming"] = prefs.nightModeDimmingEnabled;
    data["dayBrightness"] = map(constrain(prefs.dayBrightness, 0, 255), 0, 255, 1, 25);
    data["nightBrightness"] = map(constrain(prefs.nightBrightness, 0, 255), 0, 255, 1, 25);
    data["nightStartHour"] = prefs.nightStartHour;
    data["nightEndHour"] = prefs.nightEndHour;
    
    // Add sensorhub data
    data["useSensorhub"] = prefs.useSensorhub;
    data["sensorhubUsername"] = prefs.sensorhubUsername;
    // Don't send back the actual password, just whether one exists
    data["hasSensorhubPassword"] = prefs.sensorhubPassword.length() > 0;
    
    // Add MQTT settings
    data["mqttPublishEnabled"] = prefs.mqttPublishEnabled;
    data["mqttBrokerAddress"] = prefs.mqttBrokerAddress;
    data["mqttUsername"] = prefs.mqttUsername;
    // Don't send back the actual password, just whether one exists
    data["hasMqttPassword"] = prefs.mqttPassword.length() > 0;
    data["mqttPublishInterval"] = prefs.mqttPublishInterval;
    
    // Cache the serialized response
    cachedPreferencesJson = "";
    serializeJson(doc, cachedPreferencesJson);
    lastPreferencesCacheTime = now;
    
    server->send(200, "application/json", cachedPreferencesJson);
}

void handleSetPreferences() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
        return;
    }

    String jsonData = server->arg("plain");
    StaticJsonDocument<1024> doc; // Increased size for MQTT settings
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    try {
        // Get existing preferences first to preserve values not being updated
        DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
        
        // Update display settings
        if (doc.containsKey("nightDimming"))
            prefs.nightModeDimmingEnabled = doc["nightDimming"].as<bool>();
            
        if (doc.containsKey("dayBrightness")) {
            int dayBrightness = doc["dayBrightness"].as<int>();
            if (dayBrightness < 1 || dayBrightness > 25) {
                server->send(400, "application/json", 
                    "{\"success\":false,\"error\":\"Brightness must be between 1 and 25\"}");
                return;
            }
            prefs.dayBrightness = map(dayBrightness, 1, 25, 1, 75);
        }
        
        if (doc.containsKey("nightBrightness")) {
            int nightBrightness = doc["nightBrightness"].as<int>();
            if (nightBrightness < 1 || nightBrightness > 25) {
                server->send(400, "application/json", 
                    "{\"success\":false,\"error\":\"Brightness must be between 1 and 25\"}");
                return;
            }
            prefs.nightBrightness = map(nightBrightness, 1, 25, 1, 75);
        }
        
        if (doc.containsKey("nightStartHour"))
            prefs.nightStartHour = doc["nightStartHour"].as<uint8_t>();
            
        if (doc.containsKey("nightEndHour"))
            prefs.nightEndHour = doc["nightEndHour"].as<uint8_t>();
        
        if (prefs.nightStartHour >= 24 || prefs.nightEndHour >= 24) {
            server->send(400, "application/json", 
                "{\"success\":false,\"error\":\"Hours must be between 0 and 23\"}");
            return;
        }
        
        // Handle sensorhub settings
        if (doc.containsKey("useSensorhub"))
            prefs.useSensorhub = doc["useSensorhub"].as<bool>();
            
        if (doc.containsKey("sensorhubUsername"))
            prefs.sensorhubUsername = doc["sensorhubUsername"].as<String>();
            
        // Only update password if provided (don't clear existing password)
        if (doc.containsKey("sensorhubPassword") && doc["sensorhubPassword"].as<String>().length() > 0)
            prefs.sensorhubPassword = doc["sensorhubPassword"].as<String>();
        
        // Handle MQTT settings
        if (doc.containsKey("mqttPublishEnabled"))
            prefs.mqttPublishEnabled = doc["mqttPublishEnabled"].as<bool>();
            
        if (doc.containsKey("mqttBrokerAddress"))
            prefs.mqttBrokerAddress = doc["mqttBrokerAddress"].as<String>();
            
        if (doc.containsKey("mqttUsername"))
            prefs.mqttUsername = doc["mqttUsername"].as<String>();
            
        // Only update MQTT password if provided
        if (doc.containsKey("mqttPassword") && doc["mqttPassword"].as<String>().length() > 0)
            prefs.mqttPassword = doc["mqttPassword"].as<String>();
            
        if (doc.containsKey("mqttPublishInterval")) {
            uint16_t interval = doc["mqttPublishInterval"].as<uint16_t>();
            // Validate interval (minimum 10 seconds, maximum 3600 seconds)
            if (interval < 10 || interval > 3600) {
                server->send(400, "application/json", 
                    "{\"success\":false,\"error\":\"Publish interval must be between 10 and 3600 seconds\"}");
                return;
            }
            prefs.mqttPublishInterval = interval;
        }
        
        // Save preferences (this also updates the cache)
        PreferencesManager::saveDisplayPreferences(prefs);
        
        // Invalidate the cached JSON response
        lastPreferencesCacheTime = 0;
        
        // Set display preferences directly (optional)
        DisplayHandler* display = g_state->getDisplay();
        if (display) {
            display->setDisplayPreferences(prefs);
        }
        
        // Update BabelSensor if it exists (assumes global access to it)
        extern BabelSensor babelSensor;
        if (prefs.useSensorhub) {
            babelSensor.setEnabled(true);
            if (prefs.sensorhubUsername.length() > 0) {
                babelSensor.loginWithStoredCredentials();
            }
        } else {
            babelSensor.setEnabled(false);
        }
        
        if (prefs.mqttPublishEnabled) {
            // Update MQTT client with new settings
            // mqttClient.setServer(prefs.mqttBrokerAddress.c_str(), MQTT_PORT);
            // if (prefs.mqttUsername.length() > 0) {
            //     mqttClient.setCredentials(prefs.mqttUsername.c_str(), prefs.mqttPassword.c_str());
            // }
            // mqttClient.setPublishInterval(prefs.mqttPublishInterval * 1000); // Convert to milliseconds
        }

        server->send(200, "application/json", "{\"success\":true}");

    } catch (const std::exception& e) {
        String errorMsg = "{\"success\":false,\"error\":\"Internal error: ";
        errorMsg += e.what();
        errorMsg += "\"}";
        server->send(500, "application/json", errorMsg);
    }
}

void handleOptionsPreferences() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;
    
    addCorsHeaders(server);
    server->send(204);
}

void handleCaptivePortal() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    if (webManager.isInAPMode() && server->hostHeader() != WiFi.softAPIP().toString()) {
        server->sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server->send(302, "text/plain", "");
    } else {
        handleRoot();
    }
}

void handleIcon() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    String path = server->uri();
    String iconName = path.substring(path.lastIndexOf('/') + 1);
    
    addCorsHeaders(server);
    server->sendHeader("Cache-Control", "public, max-age=31536000");
    server->sendHeader("Content-Type", "image/svg+xml");
    
    const char* iconContent = nullptr;
    
    if (iconName == "lock") {
        iconContent = LOCK_ICON;
    } 
    else if (iconName == "signal-1") {
        iconContent = SIGNAL_WEAK;
    }
    else if (iconName == "signal-2") {
        iconContent = SIGNAL_FAIR;
    }
    else if (iconName == "signal-3") {
        iconContent = SIGNAL_GOOD;
    }
    else if (iconName == "signal-4") {
        iconContent = SIGNAL_STRONG;
    }
    
    if (iconContent) {
        server->send(200, "image/svg+xml", iconContent);
    } else {
        server->send(404, "text/plain", "Icon not found");
    }
}

void handleGetRelayState() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);
    
    auto& relayHandler = RelayControlHandler::getInstance();
    String response;
    
    if (!relayHandler.getRelayStates(response)) {
        server->send(500, "application/json", "{\"error\":\"Failed to fetch relay status\"}");
        return;
    }

    // Pass the response directly since it's already in JSON format
    server->send(200, "application/json", response);
}

void handleSetRelayState() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
        return;
    }

    String jsonData = server->arg("plain");
    Serial.printf("Received relay command: %s\n", jsonData.c_str());

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing relay_id or state\"}");
        return;
    }

    uint8_t relayId = doc["relay_id"].as<uint8_t>();
    String stateStr = doc["state"].as<String>();
    Serial.printf("Setting relay %d to %s\n", relayId, stateStr.c_str());

    auto& relayHandler = RelayControlHandler::getInstance();
    RelayState newState = (stateStr == "ON") ? RelayState::ON : RelayState::OFF;
    
    // Use setState which already handles authentication internally
    if (relayHandler.setState(relayId, newState)) {
        server->send(200, "application/json", "{\"success\":true}");
    } else {
        server->send(500, "application/json", 
            "{\"success\":false,\"error\":\"Failed to set relay state\"}");
    }
}

void handleRelayControl() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    if (server->method() == HTTP_OPTIONS) {
        addCorsHeaders(server);
        server->send(204);
        return;
    }

    if (server->method() == HTTP_GET) {
        handleGetRelayState();
        return;
    }

    if (server->method() == HTTP_POST) {
        handleSetRelayState();
        return;
    }

    server->send(405, "application/json", "{\"success\":false,\"error\":\"Method not allowed\"}");
}

void setupWebHandlers() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    // Basic handlers
    server->on("/", HTTP_GET, handleRoot);
    server->onNotFound(handleCaptivePortal);

    // WiFi configuration handlers
    server->on("/scan", HTTP_GET, handleScan);
    server->on("/connect", HTTP_POST, handleConnect);
    server->on("/api/wifi/status", HTTP_GET, handleWiFiStatus);
    server->on("/api/wifi/reconnect", HTTP_POST, handleWiFiReconnect);
    server->on("/api/wifi/credentials", HTTP_POST, handleSetWifiCredentials);

    // Device preferences handlers
    server->on("/api/preferences", HTTP_GET, handleGetPreferences);
    server->on("/api/preferences", HTTP_POST, handleSetPreferences);
    server->on("/api/preferences", HTTP_OPTIONS, handleOptionsPreferences);

    // Relay control handlers
    server->on("/api/relay", HTTP_GET, handleGetRelayState);
    server->on("/api/relay", HTTP_POST, handleSetRelayState);
    server->on("/api/relay", HTTP_OPTIONS, []() {
        auto& webManager = WebServerManager::getInstance();
        WebServer* server = webManager.getServer();
        if (!server) return;
        
        addCorsHeaders(server);
        server->send(204);
    });

    // Icon handler
    server->on("/icon.svg", HTTP_GET, handleIcon);

    Serial.println("Web handlers initialized");
}

void handleSetWifiCredentials() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
        return;
    }

    String jsonData = server->arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("ssid")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"SSID is required\"}");
        return;
    }

    String ssid = doc["ssid"].as<String>();
    String password = doc.containsKey("password") ? doc["password"].as<String>() : "";
    
    // Add CORS headers
    addCorsHeaders(server);
    
    // Prepare success response
    String response = "{\"success\":true,\"message\":\"WiFi credentials updated. The device will now attempt to connect to the new network.\"}";
    server->send(200, "application/json", response);
    server->client().stop(); // Ensure the response is sent
    delay(500); // Short delay
    
    // Attempt to connect with new credentials
    auto& wifiManager = WiFiConnectionManager::getInstance();
    
    // Disconnect first
    wifiManager.disconnect(false);
    delay(500);
    
    // Store and connect with new credentials
    if (wifiManager.connect(ssid, password)) {
        Serial.println("[WEB] Successfully connected with new credentials");
        // Allow normal operation to continue
    } else {
        Serial.println("[WEB] Failed to connect with new credentials");
        // The WiFiConnectionManager will handle reconnection or portal mode
    }
}

void handleWiFiStatus() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);
    
    StaticJsonDocument<256> doc;
    auto& wifiManager = WiFiConnectionManager::getInstance();
    
    // Get current connection status
    doc["connected"] = wifiManager.isConnected();
    doc["ssid"] = wifiManager.getSSID();
    doc["ip_address"] = wifiManager.getIPAddress();
    doc["signal_strength"] = WiFi.RSSI();
    doc["status"] = static_cast<int>(wifiManager.getStatus());
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void handleWiFiReconnect() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);
    
    auto& wifiManager = WiFiConnectionManager::getInstance();
    
    // Reset the connection (disconnect and reconnect)
    bool success = wifiManager.resetConnection();
    
    if (success) {
        server->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi connection reset successfully\"}");
    } else {
        server->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to reset WiFi connection\"}");
    }
}

// Add WiFi scan handler
void handleScan() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);
    
    // Perform WiFi scan
    int networks = WiFi.scanNetworks();
    
    DynamicJsonDocument doc(2048);
    JsonArray networksArray = doc.to<JsonArray>();
    
    for (int i = 0; i < networks; i++) {
        JsonObject networkObj = networksArray.createNestedObject();
        networkObj["ssid"] = WiFi.SSID(i);
        networkObj["rssi"] = WiFi.RSSI(i);
        networkObj["encrypted"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        networkObj["channel"] = WiFi.channel(i);
        
        // Add a small delay to prevent watchdog resets during large scan results
        if (i % 5 == 0) {
            esp_task_wdt_reset();
            delay(10);
        }
    }
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
    
    // Clean up scan results
    WiFi.scanDelete();
}