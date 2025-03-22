#pragma once
#include "PreferenceStorage.h"
#include "SystemDefinitions.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <functional>

class PreferencesManager {
public:
    using PreferencesChangedCallback = std::function<void(const DisplayPreferences&)>;
    
    static void begin();
    static void saveDisplayPreferences(const DisplayPreferences& prefs);
    static DisplayPreferences loadDisplayPreferences();
    static void setPreferencesChangedCallback(PreferencesChangedCallback callback);
    
    // New methods for optimized preference handling
    static bool isPreferencesLoaded();
    static void refreshPreferences();

private:
    static PreferenceStorage* storage;
    static SemaphoreHandle_t prefsMutex;
    static PreferencesChangedCallback onPreferencesChanged;
    
    // Cache for preferences to avoid slow file operations
    static DisplayPreferences cachedPreferences;
    static bool preferencesLoaded;
    static unsigned long lastPrefsLoadTime;
};