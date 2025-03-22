#include "WiFiConnectionManager.h"
#include "config.h"
#include <Preferences.h>

// Initialize static members
WiFiConnectionManager* WiFiConnectionManager::_instance = nullptr;

WiFiConnectionManager& WiFiConnectionManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new WiFiConnectionManager();
    }
    return *_instance;
}

WiFiConnectionManager::WiFiConnectionManager() 
    : _status(WiFiStatus::DISCONNECTED)
    , _ipAddress("")
    , _ssid("")
    , _lastConnectionAttempt(0)
    , _reconnectInterval(DEFAULT_RECONNECT_INTERVAL)
    , _reconnectCount(0)
    , _maxReconnectAttempts(MAX_RECONNECT_ATTEMPTS)
    , _statusCallback(nullptr)
    , _initialized(false)
{
    // Initialize
}

bool WiFiConnectionManager::begin() {
    if (_initialized) return true;
    
    Serial.println("[WIFI] Initializing WiFi Connection Manager");
    
    // Setup event handlers
    setupEventHandlers();
    
    _initialized = true;
    return true;
}

bool WiFiConnectionManager::connect(const String& ssid, const String& password, uint32_t timeout) {
    if (!_initialized) {
        if (!begin()) {
            return false;
        }
    }
    
    // Store credentials if successful
    bool result = internalConnect(ssid, password, timeout);
    if (result) {
        storeCredentials(ssid, password);
    }
    
    return result;
}

bool WiFiConnectionManager::connectWithStoredCredentials(uint32_t timeout) {
    if (!hasStoredCredentials()) {
        Serial.println("[WIFI] No stored credentials available");
        updateStatus(WiFiStatus::CONNECTION_FAILED);
        return false;
    }
    
    // Load credentials
    Preferences prefs;
    prefs.begin("wifi_config", true); // Read-only
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");
    prefs.end();
    
    if (ssid.isEmpty()) {
        Serial.println("[WIFI] Empty SSID in stored credentials");
        updateStatus(WiFiStatus::CONNECTION_FAILED);
        return false;
    }
    
    Serial.printf("[WIFI] Connecting with stored credentials to SSID: %s\n", ssid.c_str());
    return internalConnect(ssid, password, timeout);
}

void WiFiConnectionManager::disconnect(bool clearCreds) {
    Serial.println("[WIFI] Disconnecting from WiFi network");
    
    // Disconnect WiFi
    WiFi.disconnect(true);
    delay(100);
    
    // Clear credentials if requested
    if (clearCreds) {
        clearCredentials();
    }
    
    _ssid = "";
    _ipAddress = "";
    updateStatus(WiFiStatus::DISCONNECTED);
}

bool WiFiConnectionManager::resetConnection() {
    Serial.println("[WIFI] Resetting WiFi connection");
    
    // Disconnect first
    disconnect(false);
    delay(200);
    
    // Then try to reconnect with stored credentials
    return connectWithStoredCredentials();
}

bool WiFiConnectionManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED && validateConnection();
}

WiFiStatus WiFiConnectionManager::getStatus() const {
    // Update status based on actual connection state
    if (_status == WiFiStatus::CONNECTED && !isConnected()) {
        // We're not actually connected despite our status
        const_cast<WiFiConnectionManager*>(this)->updateStatus(WiFiStatus::DISCONNECTED);
    }
    
    return _status;
}

String WiFiConnectionManager::getIPAddress() const {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return _ipAddress;
}

String WiFiConnectionManager::getSSID() const {
    if (isConnected()) {
        return WiFi.SSID();
    }
    return _ssid;
}

bool WiFiConnectionManager::hasStoredCredentials() const {
    Preferences prefs;
    prefs.begin("wifi_config", true); // Read-only
    bool hasCredentials = prefs.isKey("ssid");
    prefs.end();
    return hasCredentials;
}

void WiFiConnectionManager::storeCredentials(const String& ssid, const String& password) {
    if (ssid.isEmpty()) return;
    
    Preferences prefs;
    prefs.begin("wifi_config", false); // Read-write
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
    
    Serial.printf("[WIFI] Stored credentials for SSID: %s\n", ssid.c_str());
}

void WiFiConnectionManager::clearCredentials() {
    Preferences prefs;
    prefs.begin("wifi_config", false); // Read-write
    prefs.remove("ssid");
    prefs.remove("password");
    prefs.end();
    
    Serial.println("[WIFI] Cleared stored credentials");
}

String WiFiConnectionManager::getStoredSSID() const {
    Preferences prefs;
    prefs.begin("wifi_config", true); // Read-only
    String ssid = prefs.getString("ssid", "");
    prefs.end();
    return ssid;
}

void WiFiConnectionManager::setStatusCallback(WiFiStatusCallback callback) {
    _statusCallback = callback;
}

void WiFiConnectionManager::loop() {
    // Check connection status and handle reconnection
    if (_status == WiFiStatus::DISCONNECTED) {
        unsigned long now = millis();
        if (now - _lastConnectionAttempt >= _reconnectInterval) {
            _lastConnectionAttempt = now;
            
            if (_reconnectCount < _maxReconnectAttempts) {
                Serial.printf("[WIFI] Attempting reconnection (%d/%d)...\n", 
                             _reconnectCount + 1, _maxReconnectAttempts);
                
                _reconnectCount++;
                
                if (connectWithStoredCredentials()) {
                    Serial.println("[WIFI] Reconnection successful");
                    _reconnectCount = 0;
                    _reconnectInterval = DEFAULT_RECONNECT_INTERVAL;
                } else {
                    Serial.println("[WIFI] Reconnection failed");
                    
                    // Increase backoff time
                    _reconnectInterval = std::min<unsigned long>(_reconnectInterval * 2, MAX_RECONNECT_INTERVAL);
                    Serial.printf("[WIFI] Next attempt in %lu ms\n", _reconnectInterval);
                    
                    if (_reconnectCount >= _maxReconnectAttempts) {
                        Serial.println("[WIFI] Maximum reconnection attempts reached");
                        updateStatus(WiFiStatus::CONNECTION_FAILED);
                    }
                }
            }
        }
    }
    
    // Handle connection state check
    if (_status == WiFiStatus::CONNECTED && !isConnected()) {
        Serial.println("[WIFI] Connection lost");
        updateStatus(WiFiStatus::DISCONNECTED);
        _reconnectCount = 0;
        _reconnectInterval = DEFAULT_RECONNECT_INTERVAL;
        _lastConnectionAttempt = millis();
    }
}

void WiFiConnectionManager::dumpStatus() const {
    Serial.println("\n===== WiFi Status =====");
    Serial.printf("Current status: %d\n", static_cast<int>(_status));
    Serial.printf("Connected: %s\n", isConnected() ? "YES" : "NO");
    Serial.printf("WiFi status: %d\n", WiFi.status());
    Serial.printf("SSID: %s\n", getSSID().c_str());
    Serial.printf("IP address: %s\n", getIPAddress().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("Stored credentials: %s\n", hasStoredCredentials() ? "YES" : "NO");
    Serial.printf("Reconnect count: %d/%d\n", _reconnectCount, _maxReconnectAttempts);
    Serial.printf("Reconnect interval: %lu ms\n", _reconnectInterval);
    Serial.println("======================\n");
}

bool WiFiConnectionManager::internalConnect(const String& ssid, const String& password, uint32_t timeout) {
    // Ensure we're disconnected first
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
        delay(100);
    }
    
    updateStatus(WiFiStatus::CONNECTING);
    _lastConnectionAttempt = millis();
    
    // Configure WiFi
    WiFi.disconnect(true, true);  // Reset Wi-Fi configuration
    WiFi.setHostname(MQTT_CLIENT_ID);
    WiFi.mode(WIFI_STA);
    Serial.printf("[WIFI] Setting hostname to: %s\n", MQTT_CLIENT_ID);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    // Disable power saving for better reliability
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // Connect
    Serial.printf("[WIFI] Connecting to SSID: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        
        // Check for timeout
        if (millis() - startTime > timeout) {
            Serial.println("\n[WIFI] Connection timeout");
            WiFi.disconnect(true);
            updateStatus(WiFiStatus::CONNECTION_FAILED);
            return false;
        }
        
        // Feed the watchdog if needed
        esp_task_wdt_reset();
    }
    
    // Extra wait for IP address
    startTime = millis();
    while (WiFi.localIP().toString() == "0.0.0.0") {
        delay(500);
        Serial.print("I");
        
        // Check for timeout
        if (millis() - startTime > 5000) {
            Serial.println("\n[WIFI] DHCP timeout");
            WiFi.disconnect(true);
            updateStatus(WiFiStatus::CONNECTION_FAILED);
            return false;
        }
        
        // Feed the watchdog if needed
        esp_task_wdt_reset();
    }
    
    // Connection successful
    _ipAddress = WiFi.localIP().toString();
    _ssid = WiFi.SSID();
    
    Serial.printf("\n[WIFI] Connected successfully to %s, IP address: %s\n", 
                 _ssid.c_str(), _ipAddress.c_str());
    
    updateStatus(WiFiStatus::CONNECTED);
    return true;
}

bool WiFiConnectionManager::validateConnection() const {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    IPAddress ip = WiFi.localIP();
    if (ip.toString() == "0.0.0.0") {
        return false;
    }
    
    return true;
}

void WiFiConnectionManager::updateStatus(WiFiStatus newStatus) {
    if (_status == newStatus) return;
    
    _status = newStatus;
    
    // Call status callback if set
    if (_statusCallback) {
        _statusCallback(_status, _ipAddress);
    }
}

void WiFiConnectionManager::setupEventHandlers() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
            case SYSTEM_EVENT_STA_CONNECTED:
                Serial.println("[WIFI] Connected to access point");
                // Don't update status yet, wait for IP
                break;
                
            case SYSTEM_EVENT_STA_GOT_IP:
                _ipAddress = WiFi.localIP().toString();
                _ssid = WiFi.SSID();
                Serial.printf("[WIFI] Got IP address: %s\n", _ipAddress.c_str());
                updateStatus(WiFiStatus::CONNECTED);
                _reconnectCount = 0;
                break;
                
            case SYSTEM_EVENT_STA_DISCONNECTED:
                if (_status == WiFiStatus::CONNECTED) {
                    Serial.println("[WIFI] Disconnected from access point");
                    updateStatus(WiFiStatus::DISCONNECTED);
                    _lastConnectionAttempt = millis();
                }
                break;
                
            case SYSTEM_EVENT_WIFI_READY:
                Serial.println("[WIFI] WiFi subsystem ready");
                break;
                
            case SYSTEM_EVENT_STA_START:
                Serial.println("[WIFI] WiFi client started");
                break;
                
            case SYSTEM_EVENT_STA_STOP:
                Serial.println("[WIFI] WiFi client stopped");
                updateStatus(WiFiStatus::DISCONNECTED);
                break;
                
            case SYSTEM_EVENT_STA_LOST_IP:
                Serial.println("[WIFI] Lost IP address");
                updateStatus(WiFiStatus::DISCONNECTED);
                break;
        }
    });
}