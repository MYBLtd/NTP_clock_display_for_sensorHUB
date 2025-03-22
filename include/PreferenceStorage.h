#pragma once
#include <Arduino.h>
#include <SPIFFS.h>

// Abstract base class
class PreferenceStorage {
public:
    static const char* BASE_PATH; 
    virtual bool begin(const char* name, bool readOnly) = 0;
    virtual bool putString(const char* key, const char* value) = 0;
    virtual String getString(const char* key, const char* defaultValue) = 0;
    virtual bool putUChar(const char* key, uint8_t value) = 0;
    virtual uint8_t getUChar(const char* key, uint8_t defaultValue) = 0;
    virtual bool putBool(const char* key, bool value) = 0;
    virtual bool getBool(const char* key, bool defaultValue) = 0;
    virtual ~PreferenceStorage() = default;
};

// SPIFFS implementation
class SPIFFSPreferenceStorage : public PreferenceStorage {
private:
    static const char* BASE_PATH;
    String prefNamespace;

    String getFilePath(const char* key) const;
    void ensureDirectory() const;

public:
    static const char* getBasePath() {
        return BASE_PATH;
    }
    bool begin(const char* name, bool readOnly) override;
    bool putString(const char* key, const char* value) override;
    String getString(const char* key, const char* defaultValue) override;
    bool putUChar(const char* key, uint8_t value) override;
    uint8_t getUChar(const char* key, uint8_t defaultValue) override;
    bool putBool(const char* key, bool value) override;
    bool getBool(const char* key, bool defaultValue) override;
};