#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

class OTAManager {
public:
    static OTAManager& getInstance() {
        static OTAManager instance;
        return instance;
    }

    bool begin();
    bool update(const char* url);
    int getProgress() const { return _progress; }
    
    // Delete copy constructor and assignment operator
    OTAManager(const OTAManager&) = delete;
    OTAManager& operator=(const OTAManager&) = delete;

private:
    OTAManager() {} // Private constructor for singleton
    
    static void printProgress(size_t prg, size_t sz);
    int _progress = 0;
    bool _initialized = false;
};