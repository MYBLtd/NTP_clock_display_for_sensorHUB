#include "PreferencesManager.h"
#include "config.h"
#include <SPIFFS.h>

// Initialize static members
PreferenceStorage* PreferencesManager::storage = nullptr;
SemaphoreHandle_t PreferencesManager::prefsMutex = nullptr;
PreferencesManager::PreferencesChangedCallback PreferencesManager::onPreferencesChanged = nullptr;
DisplayPreferences PreferencesManager::cachedPreferences;
bool PreferencesManager::preferencesLoaded = false;
unsigned long PreferencesManager::lastPrefsLoadTime = 0;

void PreferencesManager::begin() {
    // Enhanced SPIFFS initialization with detailed logging
    if (!SPIFFS.begin(true)) {
        Serial.println("[CRITICAL] Failed to mount SPIFFS filesystem");
        return;
    }

    // Create base preferences directory if it doesn't exist
    if (!SPIFFS.exists(SPIFFSPreferenceStorage::getBasePath())) {
        if (SPIFFS.mkdir(SPIFFSPreferenceStorage::getBasePath())) {
            Serial.println("[INFO] Created base preferences directory");
        } else {
            Serial.println("[ERROR] Failed to create base preferences directory");
        }
    }

    prefsMutex = xSemaphoreCreateMutex();
    if (!prefsMutex) {
        Serial.println("[CRITICAL] Failed to create preferences mutex");
        return;
    }

    if (!storage) {
        storage = new SPIFFSPreferenceStorage();
    }
    
    if (!storage) {
        Serial.println("[CRITICAL] Failed to create preferences storage");
        return;
    }

    if (!storage->begin("display", false)) {
        Serial.println("[ERROR] Failed to begin preferences storage");
        return;
    }

    // Load preferences into cache on startup
    loadDisplayPreferences();

    Serial.println("[SUCCESS] Preferences system initialized");
}

void PreferencesManager::setPreferencesChangedCallback(PreferencesChangedCallback callback) {
    onPreferencesChanged = callback;
}

void PreferencesManager::saveDisplayPreferences(const DisplayPreferences& prefs) {
    if (!storage || !prefsMutex) {
        Serial.println("Preferences system not initialized");
        return;
    }

    bool mutexTaken = false;
    bool saveSuccess = false;

    // Try to get mutex with timeout
    if (xSemaphoreTake(prefsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        mutexTaken = true;
        
        // Store cached values first (fast)
        cachedPreferences = prefs;
        preferencesLoaded = true;
        
        // Store actual 1-75 range values
        uint8_t dayBright = constrain(prefs.dayBrightness, 1, 75);
        uint8_t nightBright = constrain(prefs.nightBrightness, 1, 75);
        
        // Save to storage in background
        storage->putBool("nightMode", prefs.nightModeDimmingEnabled);
        storage->putUChar("dayBright", dayBright);
        storage->putUChar("nightBright", nightBright);
        storage->putUChar("nightStart", prefs.nightStartHour);
        storage->putUChar("nightEnd", prefs.nightEndHour);
        
        // Save sensorhub credentials
        storage->putString("sensorhubUser", prefs.sensorhubUsername.c_str());
        storage->putString("sensorhubPass", prefs.sensorhubPassword.c_str());
        storage->putBool("useSensorhub", prefs.useSensorhub);
        
        // Save MQTT settings
        storage->putBool("mqttEnabled", prefs.mqttPublishEnabled);
        storage->putString("mqttBroker", prefs.mqttBrokerAddress.c_str());
        Serial.printf("MQTT: Saving broker address to preferences: '%s'\n", 
                     prefs.mqttBrokerAddress.c_str());
        
        storage->putString("mqttUser", prefs.mqttUsername.c_str());
        storage->putString("mqttPass", prefs.mqttPassword.c_str());
        storage->putUChar("mqttInterval", prefs.mqttPublishInterval);
        
        Serial.printf("Saving display preferences - Day: %d%%, Night: %d%%\n", 
                     dayBright, nightBright);
        Serial.printf("Saving sensorhub credentials - User: %s, UseAPI: %s\n",
                     prefs.sensorhubUsername.c_str(),
                     prefs.useSensorhub ? "Yes" : "No");
        Serial.printf("Saving MQTT settings - Enabled: %s, Broker: %s, Interval: %d\n",
                     prefs.mqttPublishEnabled ? "Yes" : "No",
                     prefs.mqttBrokerAddress.c_str(),
                     prefs.mqttPublishInterval);
        
        saveSuccess = true;
        xSemaphoreGive(prefsMutex);
    } else {
        Serial.println("[WARNING] Failed to acquire mutex for saving preferences");
    }

    // Notify about preferences change even if we couldn't save
    // This ensures the UI is updated even if storage failed
    if ((saveSuccess || !mutexTaken) && onPreferencesChanged) {
        onPreferencesChanged(prefs);
    }
}

DisplayPreferences PreferencesManager::loadDisplayPreferences() {
    // Use cached preferences if available and not older than 30 seconds
    unsigned long now = millis();
    if (preferencesLoaded && (now - lastPrefsLoadTime < 30000)) {
        return cachedPreferences;
    }
    
    if (!storage || !prefsMutex) {
        Serial.println("[ERROR] Preferences system not initialized when loading");
        return cachedPreferences; // Return cached values even if invalid
    }

    // Try to acquire mutex with a shorter timeout (non-blocking approach)
    if (xSemaphoreTake(prefsMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
        // Log detailed loading process
        Serial.println("[DEBUG] Loading display preferences from storage");
        
        // Load preferences from storage
        cachedPreferences.nightModeDimmingEnabled = storage->getBool("nightMode", false);
        Serial.printf("[DEBUG] Night mode dimming: %s\n", 
                     cachedPreferences.nightModeDimmingEnabled ? "Enabled" : "Disabled");
        
        uint8_t rawDayBright = storage->getUChar("dayBright", 75);
        uint8_t rawNightBright = storage->getUChar("nightBright", 10);
        
        cachedPreferences.dayBrightness = constrain(rawDayBright, 1, 75);
        cachedPreferences.nightBrightness = constrain(rawNightBright, 1, 25);
        
        cachedPreferences.nightStartHour = storage->getUChar("nightStart", 22);
        cachedPreferences.nightEndHour = storage->getUChar("nightEnd", 6);
        
        // Load sensorhub credentials
        cachedPreferences.sensorhubUsername = storage->getString("sensorhubUser", "");
        cachedPreferences.sensorhubPassword = storage->getString("sensorhubPass", "");
        cachedPreferences.useSensorhub = storage->getBool("useSensorhub", false);
        
        // Load MQTT settings
        cachedPreferences.mqttPublishEnabled = storage->getBool("mqttEnabled", false);
        cachedPreferences.mqttBrokerAddress = storage->getString("mqttBroker", MQTT_BROKER);
        cachedPreferences.mqttUsername = storage->getString("mqttUser", MQTT_USER);
        cachedPreferences.mqttPassword = storage->getString("mqttPass", MQTT_PASSWORD);
        cachedPreferences.mqttPublishInterval = storage->getUChar("mqttInterval", 60);
        
        Serial.printf("[DEBUG] Loaded preferences:\n"
                     "  Night Mode: %s\n"
                     "  Day Brightness: %d%%\n"
                     "  Night Brightness: %d%%\n"
                     "  Night Start: %d\n"
                     "  Night End: %d\n"
                     "  Using Sensorhub: %s\n"
                     "  Sensorhub Username: %s\n"
                     "  MQTT Enabled: %s\n"
                     "  MQTT Broker: %s\n"
                     "  MQTT Interval: %d\n",
                     cachedPreferences.nightModeDimmingEnabled ? "Enabled" : "Disabled",
                     cachedPreferences.dayBrightness,
                     cachedPreferences.nightBrightness,
                     cachedPreferences.nightStartHour,
                     cachedPreferences.nightEndHour,
                     cachedPreferences.useSensorhub ? "Yes" : "No",
                     cachedPreferences.sensorhubUsername.c_str(),
                     cachedPreferences.mqttPublishEnabled ? "Yes" : "No",
                     cachedPreferences.mqttBrokerAddress.c_str(),
                     cachedPreferences.mqttPublishInterval);
        
        xSemaphoreGive(prefsMutex);
        
        // Mark as loaded and update timestamp
        preferencesLoaded = true;
        lastPrefsLoadTime = now;
    } else {
        Serial.println("[WARNING] Could not acquire mutex for loading preferences - using cached values");
    }

    return cachedPreferences;
}

bool PreferencesManager::isPreferencesLoaded() {
    return preferencesLoaded;
}

// Force refresh preferences from storage
void PreferencesManager::refreshPreferences() {
    // Reset cached time to force reloading
    lastPrefsLoadTime = 0;
    // Will trigger reload on next loadDisplayPreferences call
}