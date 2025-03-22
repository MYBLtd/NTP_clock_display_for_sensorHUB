#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <functional>

// WiFi connection status
enum class WiFiStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    PORTAL_ACTIVE,
    CONNECTION_FAILED
};

// Connection event callback
using WiFiStatusCallback = std::function<void(WiFiStatus newStatus, String ipAddress)>;

class WiFiConnectionManager {
public:
    // Singleton pattern
    static WiFiConnectionManager& getInstance();
    
    // Initialize WiFi system
    bool begin();
    
    // Connect with specific credentials
    bool connect(const String& ssid, const String& password, uint32_t timeout = 30000);
    
    // Connect with stored credentials
    bool connectWithStoredCredentials(uint32_t timeout = 30000);
    
    // Stop all WiFi activity and disconnect
    void disconnect(bool clearCredentials = false);
    
    // Reset connection (disconnect and reconnect)
    bool resetConnection();
    
    // Check if connection is active
    bool isConnected() const;
    
    // Get current status
    WiFiStatus getStatus() const;
    
    // Get IP address
    String getIPAddress() const;
    
    // Get SSID
    String getSSID() const;
    
    // Check if credentials are stored
    bool hasStoredCredentials() const;
    
    // Store credentials
    void storeCredentials(const String& ssid, const String& password);
    
    // Clear stored credentials
    void clearCredentials();
    
    // Get credentials (only SSID for safety)
    String getStoredSSID() const;
    
    // Register status change callback
    void setStatusCallback(WiFiStatusCallback callback);
    
    // Run periodic maintenance tasks
    void loop();
    
    // Debug functions
    void dumpStatus() const;
    
private:
    // Private constructor for singleton
    WiFiConnectionManager();
    
    // Disable copy and assignment
    WiFiConnectionManager(const WiFiConnectionManager&) = delete;
    WiFiConnectionManager& operator=(const WiFiConnectionManager&) = delete;
    
    // Internal connection handler
    bool internalConnect(const String& ssid, const String& password, uint32_t timeout);
    
    // Check for valid IP
    bool validateConnection() const;
    
    // Update status and notify
    void updateStatus(WiFiStatus newStatus);
    
    // Setup event handlers
    void setupEventHandlers();
    
    // State variables
    WiFiStatus _status;
    String _ipAddress;
    String _ssid;
    unsigned long _lastConnectionAttempt;
    unsigned long _reconnectInterval;
    int _reconnectCount;
    int _maxReconnectAttempts;
    WiFiStatusCallback _statusCallback;
    bool _initialized;
    
    // Static instance for singleton
    static WiFiConnectionManager* _instance;
    
    // Constants
    static const uint32_t DEFAULT_RECONNECT_INTERVAL = 30000; // 30 seconds
    static const uint32_t MAX_RECONNECT_INTERVAL = 300000;    // 5 minutes
    static const int MAX_RECONNECT_ATTEMPTS = 5;
};