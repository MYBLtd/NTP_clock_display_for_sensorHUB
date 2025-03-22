#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WString.h>
#include "PreferencesManager.h"

class BabelSensor {
    public:
        BabelSensor(const char* serverUrl);
        bool init();
        bool login(const char* username, const char* password);
        float getRemoteTemperature();
        bool isAuthenticated() const { return !authToken.isEmpty(); }
        
        // New methods for working with stored credentials
        bool loginWithStoredCredentials();
        bool updateCredentials(const String& username, const String& password);
        void setEnabled(bool enabled);
        bool isEnabled() const;
        String getAuthToken() const {
            return authToken;
        }
    private:
        String serverUrl;
        String authToken;
        unsigned long lastUpdate;
        float lastTemperature;
        bool enabled;
        static constexpr unsigned long UPDATE_INTERVAL = 30000; // 30 seconds
        // Define our own TOKEN_REFRESH_INTERVAL instead of using the one from config.h that's causing issues
        static constexpr unsigned long TOKEN_REFRESH_INTERVAL_MS = 3600000; // 1 hour in milliseconds
        unsigned long lastTokenRefresh;
        
        // Helper method to ensure authentication
        bool ensureAuthenticated();
};