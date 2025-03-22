#include "WebServerManager.h"
#include "WebHandlers.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "WiFiConnectionManager.h"

class WebServerManager::WiFiEventHandler {
public:
    uint8_t retry_count = 0;
};

WebServerManager::WebServerManager() 
    : _currentMode(ServerMode::UNDEFINED)
    , _connectionStatus(ConnectionStatus::DISCONNECTED)
    , _initialized(false)
    , _lastReconnectAttempt(0)
    , _wifiHandler(new WiFiEventHandler()) {
    _preferences.begin("wifi_config", false);
    setupWiFiEventHandlers();
}

WebServerManager::~WebServerManager() {
    delete _wifiHandler;
}

void WebServerManager::setupWiFiEventHandlers() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch(event) {
            case SYSTEM_EVENT_STA_START:
                updateConnectionStatus(ConnectionStatus::CONNECTING);
                break;

            case SYSTEM_EVENT_STA_CONNECTED:
                Serial.println("WiFi connected, waiting for IP");
                break;

            case SYSTEM_EVENT_STA_GOT_IP:
                updateConnectionStatus(ConnectionStatus::CONNECTED);
                _wifiHandler->retry_count = 0;
                
                if (_currentMode == ServerMode::PORTAL) {
                    startPreferencesMode();
                }
                break;

            case SYSTEM_EVENT_STA_DISCONNECTED: {
                uint8_t reason = info.wifi_sta_disconnected.reason;
                Serial.printf("WiFi disconnected, reason: %d\n", reason);
                updateConnectionStatus(ConnectionStatus::DISCONNECTED);
                
                static uint8_t retry_count = 0;
                if (_currentMode == ServerMode::PREFERENCES) {
                    if (retry_count < 5) {
                        Serial.println("Attempting reconnection...");
                        retry_count++;
                        WiFi.begin();
                        return;
                    }
                }
                
                if (_currentMode != ServerMode::PORTAL) {
                    Serial.println("Max retries reached, switching to AP mode");
                    retry_count = 0;
                    startPortalMode();
                }
                break;
            }
        }
    });
}

bool WebServerManager::begin() {
    if (_initialized) return true;
    
    Serial.println("Starting WebServerManager initialization...");
    
    _server = std::unique_ptr<WebServer>(new WebServer(80));
    _initialized = true;

    if (WiFi.status() == WL_CONNECTED) {
        return startPreferencesMode();
    } else {
        return startPortalMode();
    }
}

void WebServerManager::handleClient() {
    if (!_server) {
        Serial.println("Error: Server is null in handleClient");
        return;
    }
    
    // Add debug logging
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 5000) {  // Log every 5 seconds
        Serial.printf("WebServer status - Mode: %d, Connected: %d\n", 
            static_cast<int>(_currentMode),
            WiFi.status() == WL_CONNECTED);
        lastLog = millis();
    }
    
    if (_dnsServer && _currentMode == ServerMode::PORTAL) {
        _dnsServer->processNextRequest();
    }
    
    _server->handleClient();
}

void WebServerManager::stop() {
    if (_server) {
        _server->stop();
    }
    stopDNSServer();
    WiFi.disconnect(true);
}

bool WebServerManager::startPortalMode() {
    if (!_initialized || !_server) return false;
    
    clearHandlers();
    stopDNSServer();
    _currentMode = ServerMode::PORTAL;
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    
    IPAddress local_IP(192,168,4,1);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("AP Config Failed");
        return false;
    }
    
    if (!WiFi.softAP(WIFI_SETUP_AP_NAME, WIFI_SETUP_PASSWORD)) {
        Serial.println("AP Start Failed");
        return false;
    }
    
    setupHandlers();  // Register all handlers
    startDNSServer();
    _server->begin();
    
    Serial.println("Portal mode started successfully");
    return true;
}


bool WebServerManager::startPreferencesMode() {
    if (!_initialized || !_server) return false;
    
    clearHandlers();
    stopDNSServer();
    _currentMode = ServerMode::PREFERENCES;
    
    setupHandlers();  // Register all handlers
    _server->begin();
    
    Serial.println("Preferences mode started successfully");
    return true;
}

void WebServerManager::setupPortalHandlers() {
    _server->on("/", HTTP_GET, handleRoot);
    _server->on("/scan", HTTP_GET, handleScan);
    _server->on("/connect", HTTP_POST, [this]() {
        this->handleConnect();
    });
    _server->on("/icon.svg", HTTP_GET, handleIcon);
    _server->onNotFound(handleCaptivePortal);
}

void WebServerManager::setupPreferencesHandlers() {
    if (!_server) return;

    // Existing handlers...

    // Add relay control handlers
    _server->on("/api/preferences/relay", HTTP_GET, [this]() {
        auto& handler = RelayControlHandler::getInstance();
        
        StaticJsonDocument<256> doc;
        JsonArray relays = doc.createNestedArray("relays");
        
        for (uint8_t i = 0; i < RelayControlHandler::NUM_RELAYS; i++) {
            RelayStatus status = handler.getRelayStatus(i);
            JsonObject relay = relays.createNestedObject();
            relay["relay_id"] = i;
            relay["state"] = status.state == RelayState::ON ? "ON" : "OFF";
            relay["override"] = status.override;
        }
        
        String response;
        serializeJson(doc, response);
        this->_server->send(200, "application/json", response);
    });

    _server->on("/api/preferences/relay", HTTP_POST, [this]() {
        if (!this->_server->hasArg("plain")) {
            this->_server->send(400, "application/json", "{\"error\":\"No data received\"}");
            return;
        }

        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, this->_server->arg("plain"));
        
        if (error) {
            this->_server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
            this->_server->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
            return;
        }

        uint8_t relayId = doc["relay_id"].as<uint8_t>();
        const char* stateStr = doc["state"].as<const char*>();
        
        if (relayId >= RelayControlHandler::NUM_RELAYS) {
            this->_server->send(400, "application/json", "{\"error\":\"Invalid relay ID\"}");
            return;
        }

        RelayState newState = (strcmp(stateStr, "ON") == 0) ? RelayState::ON : RelayState::OFF;
        RelayControlHandler::getInstance().processCommand(relayId, newState, RelayCommandSource::USER);
        
        this->_server->send(200, "application/json", "{\"success\":true}");
    });
}

void WebServerManager::setupHandlers() {
    if (!_server) return;

    // Register core handlers
    _server->on("/", HTTP_GET, handleRoot);
    _server->on("/scan", HTTP_GET, handleScan);
    _server->on("/connect", HTTP_POST, [this]() {
        this->handleConnect();
    });
    _server->on("/icon.svg", HTTP_GET, handleIcon);
    
    // Register API handlers
    _server->on("/api/preferences", HTTP_GET, handleGetPreferences);
    _server->on("/api/preferences", HTTP_POST, handleSetPreferences);
    _server->on("/api/preferences", HTTP_OPTIONS, handleOptionsPreferences);
    
    // Register relay handlers
    _server->on("/api/relay", HTTP_GET, handleGetRelayState);
    _server->on("/api/relay", HTTP_POST, handleSetRelayState);
    _server->on("/api/relay", HTTP_OPTIONS, [this]() {
        if (_server) {
            addCorsHeaders();
            _server->send(204);
        }
    });

    // Must be last - handle captive portal and 404s
    _server->onNotFound(handleCaptivePortal);
}


void WebServerManager::addCorsHeaders() {
    if (!_server) return;
    _server->sendHeader("Access-Control-Allow-Origin", "*");
    _server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WebServerManager::setWiFiCredentials(const String& ssid, const String& password) {
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
}

bool WebServerManager::hasStoredCredentials() {
    String ssid = _preferences.getString("ssid", "");
    return ssid.length() > 0;
}

void WebServerManager::clearCredentials() {
    _preferences.remove("ssid");
    _preferences.remove("password");
}

bool WebServerManager::connectWithStoredCredentials() {
    String ssid = _preferences.getString("ssid", "");
    String password = _preferences.getString("password", "");
    
    if (ssid.length() == 0) return false;
    
    Serial.printf("Attempting to connect to SSID: %s\n", ssid.c_str());
    
    WiFi.disconnect(true, true);  // Reset Wi-Fi configuration
    WiFi.setHostname(MQTT_CLIENT_ID);
    WiFi.mode(WIFI_STA);
    Serial.printf("[WIFI] Setting hostname to: %s\n", MQTT_CLIENT_ID);
    // More robust WiFi configuration
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    // Disable WiFi power saving for more reliable connections
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // Increase timeout to 20 seconds (from 10)
    unsigned long timeout = 20000;
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.println("Waiting for WiFi connection...");
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connection attempt timed out");
        return false;
    }
    
    // Wait up to 5 more seconds for DHCP specifically
    startAttempt = millis();
    while (WiFi.localIP().toString() == "0.0.0.0" && millis() - startAttempt < 5000) {
        delay(500);
        Serial.print("Waiting for IP address...");
    }
    
    if (!validateConnection()) {
        Serial.println("Connection validation failed");
        return false;
    }
    
    Serial.printf("Successfully connected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

bool WebServerManager::reconnect() {
    if (!hasStoredCredentials()) return false;
    
    updateConnectionStatus(ConnectionStatus::CONNECTING);
    return connectWithStoredCredentials();
}

void WebServerManager::updateConnectionStatus(ConnectionStatus status) {
    if (_connectionStatus != status) {
        _connectionStatus = status;
        if (_statusCallback) {
            _statusCallback(status);
        }
    }
}

bool WebServerManager::validateConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connection validation failed: WiFi not connected");
        return false;
    }
    
    IPAddress ip = WiFi.localIP();
    if (ip[0] == 0) {
        Serial.println("Connection validation failed: Invalid IP address");
        return false;
    }
    
    // Print detailed connection info for debugging
    Serial.printf("Connection validated. IP: %s, Gateway: %s, Subnet: %s\n",
        WiFi.localIP().toString().c_str(),
        WiFi.gatewayIP().toString().c_str(),
        WiFi.subnetMask().toString().c_str());
    
    // Ensure auto-reconnect is on
    WiFi.setAutoReconnect(true);
    
    return true;
}

void WebServerManager::clearHandlers() {
    if (_server) {
        _server->stop();
        _server = std::unique_ptr<WebServer>(new WebServer(80));
    }
}

void WebServerManager::startDNSServer() {
    stopDNSServer();
    _dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
    _dnsServer->start(53, "*", WiFi.softAPIP());
}

void WebServerManager::stopDNSServer() {
    if (_dnsServer) {
        _dnsServer->stop();
        _dnsServer.reset();
    }
}

WebServerManager& WebServerManager::getInstance() {
    static WebServerManager instance;
    return instance;
}

bool WebServerManager::applyWiFiCredentials(const String& ssid, const String& password) {
    Serial.printf("[WEB] Applying new WiFi credentials for SSID: %s\n", ssid.c_str());
    
    // Use the WiFiConnectionManager to handle this properly
    auto& wifiManager = WiFiConnectionManager::getInstance();
    
    // Set hostname here, before any connection attempt
    WiFi.disconnect(true, true);  // Reset Wi-Fi configuration
    WiFi.setHostname(MQTT_CLIENT_ID);
    WiFi.mode(WIFI_STA);
    Serial.printf("[WEB] Setting hostname to: %s\n", MQTT_CLIENT_ID);
    
    // Store credentials first (so they're available even if connection fails)
    wifiManager.storeCredentials(ssid, password);
    
    // Attempt to connect with new credentials
    return wifiManager.connect(ssid, password);
}

void WebServerManager::handleConnect() {
    if (!_server) {
        Serial.println("[WEB] Error: Server is null in handleConnect");
        return;
    }

    String ssid = _server->arg("ssid");
    String password = _server->arg("password");
    
    if (ssid.isEmpty()) {
        _server->send(400, "text/plain", "SSID is required");
        return;
    }

    Serial.printf("[WEB] Connecting to new network: %s\n", ssid.c_str());
    
    // Create connection result page with appropriate timeout
    String html = "<html><head><meta http-equiv='refresh' content='";
    html += "15;url=/'><title>WiFi Connection</title></head>";
    html += "<body><h1>Connecting to WiFi Network</h1>";
    html += "<p>The device is now trying to connect to: <strong>" + ssid + "</strong></p>";
    html += "<p>If connection is successful, the device will restart in normal mode.</p>";
    html += "<p>Please wait about 15 seconds...</p>";
    html += "<p>If you cannot connect after 30 seconds, the device will return to setup mode.</p>";
    html += "</body></html>";
    
    // Send response immediately before attempting connection
    _server->send(200, "text/html", html);
    delay(500); // Give time for the response to be sent
    
    // Apply the new WiFi credentials and connect
    if (applyWiFiCredentials(ssid, password)) {
        Serial.println("[WEB] WiFi connection successful, restarting...");
        delay(2000); // Give extra time for client to receive redirect
        ESP.restart(); // Restart to apply new configuration
    } else {
        Serial.println("[WEB] WiFi connection failed, returning to portal mode");
        // Continue in portal mode - user will be redirected back to setup page
    }
}

void WebServerManager::handleWiFiSettingsUpdate() {
    if (!_server) return;
    
    String ssid = _server->arg("wifi_ssid");
    String password = _server->arg("wifi_password");
    
    if (ssid.isEmpty()) {
        _server->send(400, "application/json", "{\"success\":false,\"error\":\"SSID is required\"}");
        return;
    }
    
    // Prepare response
    String html = "<html><head><meta http-equiv='refresh' content='";
    html += "15;url=/'><title>WiFi Settings Updated</title></head>";
    html += "<body><h1>WiFi Settings Updating</h1>";
    html += "<p>The device is now applying new WiFi settings and will disconnect from the current network.</p>";
    html += "<p>If connection to the new network is successful, this page will reload in 15 seconds.</p>";
    html += "<p>If you cannot reconnect after 30 seconds, please check the network settings.</p>";
    html += "</body></html>";
    
    // Send response immediately
    _server->send(200, "text/html", html);
    delay(500); // Give time for response to be sent
    
    // Apply WiFi settings after response is sent
    auto& wifiManager = WiFiConnectionManager::getInstance();
    
    // Disconnect from current network first
    wifiManager.disconnect(false);
    delay(500);
    
    // Store and connect with new credentials
    if (wifiManager.connect(ssid, password)) {
        Serial.println("[WEB] Successfully connected with new credentials");
        delay(1000); // Short delay before proceeding
    } else {
        Serial.println("[WEB] Failed to connect with new credentials");
        // Will automatically return to portal mode if needed
    }
}

bool WebServerManager::reconnectWithStoredCredentials() {
    Serial.println("[WEB] Attempting to reconnect with stored credentials");
    
    auto& wifiManager = WiFiConnectionManager::getInstance();
    return wifiManager.connectWithStoredCredentials();
}