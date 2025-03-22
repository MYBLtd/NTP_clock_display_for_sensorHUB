#pragma once

#include <DNSServer.h>
#include <WebServer.h> 
#include <memory>
#include <Preferences.h>
#include <functional>
#include "config.h"
#include "WiFiConnectionManager.h"

class WebServerManager {
public:
    static constexpr uint8_t MAX_CONNECTION_RETRIES = 3;
    
    enum class ServerMode {
        PORTAL,
        PREFERENCES,
        UNDEFINED
    };

    enum class ConnectionStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        FAILED
    };

    using StatusCallback = std::function<void(ConnectionStatus status)>;

    static WebServerManager& getInstance();
    
    ~WebServerManager();
    bool begin();
    void handleClient();
    void stop();
    
    bool startPortalMode();
    bool startPreferencesMode();
    
    WebServer* getServer() { return _server.get(); }
    ServerMode getCurrentMode() const { return _currentMode; }
    bool isPortalActive() const { return _currentMode == ServerMode::PORTAL; }
    ConnectionStatus getConnectionStatus() const { return _connectionStatus; }
    
    void setWiFiCredentials(const String& ssid, const String& password);
    bool hasStoredCredentials();
    bool connectWithStoredCredentials();
    void clearCredentials();
    bool isInAPMode() const { return _currentMode == ServerMode::PORTAL; }

    void onStatusChange(StatusCallback callback) { _statusCallback = callback; }
    bool reconnect();

    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;
    
    bool applyWiFiCredentials(const String& ssid, const String& password);
    void handleConnect();
    void handleWiFiSettingsUpdate();
    bool reconnectWithStoredCredentials();

private:
    class WiFiEventHandler;
    
    WebServerManager();
    void setupPortalHandlers();
    void setupPreferencesHandlers();
    void clearHandlers();
    void startDNSServer();
    void stopDNSServer();
    void updateConnectionStatus(ConnectionStatus status);
    bool validateConnection();
    void setupWiFiEventHandlers();
    void setupHandlers();
    void addCorsHeaders();  // Only declared once here

    std::unique_ptr<WebServer> _server;
    std::unique_ptr<DNSServer> _dnsServer;
    Preferences _preferences;
    ServerMode _currentMode;
    ConnectionStatus _connectionStatus;
    StatusCallback _statusCallback;
    bool _initialized;
    unsigned long _lastReconnectAttempt;
    WiFiEventHandler* _wifiHandler;

    static constexpr unsigned long RECONNECT_INTERVAL = 30000;
    static constexpr unsigned long CONNECTION_TIMEOUT = 10000;
    
};