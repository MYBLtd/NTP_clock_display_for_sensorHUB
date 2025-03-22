#include "OTAManager.h"
#include "config.h"

bool OTAManager::begin() {
    if (_initialized) {
        return true;
    }
    
    _initialized = true;
    return true;
}

void OTAManager::printProgress(size_t prg, size_t sz) {
    if (sz) {
        Serial.printf("Progress: %d%%\n", (prg * 100) / sz);
    }
}

bool OTAManager::update(const char* url) {
    HTTPClient http;
    http.begin(url);
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
    
    int totalLength = http.getSize();
    if (totalLength <= 0) {
        Serial.println("Error: Content length is invalid");
        http.end();
        return false;
    }
    
    Serial.printf("OTA: Starting update, size: %d bytes\n", totalLength);
    
    if (!Update.begin(totalLength)) {
        Serial.println("Error: Not enough space for update");
        http.end();
        return false;
    }
    
    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[1024] = {0};
    
    while (http.connected() && (written < totalLength)) {
        size_t size = stream->available();
        if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            written += Update.write(buff, c);
            _progress = (written * 100) / totalLength;
            printProgress(written, totalLength);
        }
        delay(1); // Small delay to prevent watchdog trigger
    }
    
    if (written != totalLength) {
        Serial.println("Error: Only wrote " + String(written) + "/" + String(totalLength) + " bytes");
        http.end();
        return false;
    }
    
    if (!Update.end()) {
        Serial.println("Error: Update.end() failed!");
        http.end();
        return false;
    }
    
    http.end();
    
    if (Update.hasError()) {
        Serial.println("Error: Update failed with error: " + String(Update.getError()));
        return false;
    }
    
    Serial.println("Update complete!");
    return true;
}