#include "PreferenceStorage.h"

// Define the static member
const char* SPIFFSPreferenceStorage::BASE_PATH = "/prefs/";

String SPIFFSPreferenceStorage::getFilePath(const char* key) const {
    return String(BASE_PATH) + prefNamespace + "/" + String(key);
}

void SPIFFSPreferenceStorage::ensureDirectory() const {
    String dir = String(BASE_PATH) + prefNamespace;
    if (!SPIFFS.exists(dir)) {
        SPIFFS.mkdir(dir);
    }
}

bool SPIFFSPreferenceStorage::begin(const char* name, bool readOnly) {
    prefNamespace = String(name);
    ensureDirectory();
    return true;
}

bool SPIFFSPreferenceStorage::putString(const char* key, const char* value) {
    // Ensure base directory exists
    const String baseDir = String(BASE_PATH) + prefNamespace;
    if (!SPIFFS.exists(baseDir)) {
        if (!SPIFFS.mkdir(baseDir)) {
            Serial.println("[ERROR] Failed to create preferences directory: " + baseDir);
            return false;
        }
    }

    String path = getFilePath(key);
    Serial.printf("[DEBUG] Writing preference: %s = %s\n", path.c_str(), value);
    
    File file = SPIFFS.open(path, "w");
    if (!file) {
        Serial.printf("[ERROR] Failed to open file for writing: %s\n", path.c_str());
        return false;
    }
    
    size_t bytesToWrite = strlen(value);
    size_t written = file.print(value);
    file.close();
    
    if (written != bytesToWrite) {
        Serial.printf("[ERROR] Write incomplete for %s. Expected %d bytes, wrote %d bytes\n", 
                      path.c_str(), bytesToWrite, written);
        return false;
    }
    
    Serial.printf("[INFO] Successfully wrote preference: %s\n", path.c_str());
    return true;
}

String SPIFFSPreferenceStorage::getString(const char* key, const char* defaultValue) {
    String path = getFilePath(key);
    
    if (!SPIFFS.exists(path)) {
        Serial.printf("[WARN] Preference file not found: %s. Using default: %s\n", 
                      path.c_str(), defaultValue);
        return String(defaultValue);
    }
    
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("[ERROR] Failed to open preference file: %s. Using default: %s\n", 
                      path.c_str(), defaultValue);
        return String(defaultValue);
    }
    
    String value = file.readString();
    file.close();
    
    if (value.isEmpty()) {
        Serial.printf("[WARN] Empty preference value for %s. Using default: %s\n", 
                      path.c_str(), defaultValue);
        return String(defaultValue);
    }
    
    Serial.printf("[DEBUG] Read preference: %s = %s\n", path.c_str(), value.c_str());
    return value;
}

bool SPIFFSPreferenceStorage::putUChar(const char* key, uint8_t value) {
    return putString(key, String(value).c_str());
}

uint8_t SPIFFSPreferenceStorage::getUChar(const char* key, uint8_t defaultValue) {
    String value = getString(key, String(defaultValue).c_str());
    return (uint8_t)value.toInt();
}

bool SPIFFSPreferenceStorage::putBool(const char* key, bool value) {
    return putString(key, value ? "1" : "0");
}

bool SPIFFSPreferenceStorage::getBool(const char* key, bool defaultValue) {
    String value = getString(key, defaultValue ? "1" : "0");
    return value == "1";
}