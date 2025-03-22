#include "DisplayHandler.h"
#include "GlobalState.h"
#include "PreferencesManager.h"
#include "SystemDefinitions.h"

const unsigned long DisplayHandler::MODE_DURATIONS[6] = {
    DISPLAY_TIME_DURATION,    // TIME
    DISPLAY_DATE_DURATION,    // DATE
    DISPLAY_TEMP_DURATION,    // TEMPERATURE
    DISPLAY_HUM_DURATION,     // HUMIDITY
    DISPLAY_PRES_DURATION,    // PRESSURE
    DISPLAY_REMOTE_DURATION   // REMOTE_TEMP
};


DisplayHandler::DisplayHandler() 
    : sr(DATA_PIN, CLOCK_PIN, LATCH_PIN),
      displayMutex(nullptr),  // Initialize to nullptr first
      displayValid(false),
      currentMode(DisplayMode::TIME),
      modeStartTime(0),
      lastUpdate(0),
      currentBrightness(255)
{
    // Create mutex with error checking
    displayMutex = xSemaphoreCreateMutex();
    if (!displayMutex) {
        Serial.println("[CRITICAL ERROR] Failed to create display mutex!");
        return;
    }
    
    Serial.printf("[INIT] Display mutex created: %p\n", displayMutex);

    // Initialize pins
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(OE_PIN, OUTPUT);
    digitalWrite(OE_PIN, HIGH);  // Disable output initially

    // Initialize display buffers under mutex protection
    if (xSemaphoreTake(displayMutex, MUTEX_TIMEOUT) == pdTRUE) {
        memset(displayBuffer, CHAR_BLANK, DISPLAY_COUNT);
        memset(dpBuffer, 0, DISPLAY_COUNT);
        displayValid = true;
        xSemaphoreGive(displayMutex);
        Serial.println("[INIT] Display buffers initialized successfully");
    } else {
        Serial.println("[CRITICAL ERROR] Could not initialize display buffers!");
    }
}

bool DisplayHandler::init() {
    Serial.println("DisplayHandler::init() - Start");

    // Ensure mutex exists
    if (!displayMutex) {
        Serial.println("[CRITICAL] Recreating lost mutex during init");
        displayMutex = xSemaphoreCreateMutex();
    }

    // Much longer mutex wait time
    const TickType_t xBlockTime = pdMS_TO_TICKS(1000);
    
    if (xSemaphoreTake(displayMutex, xBlockTime) == pdTRUE) {
        Serial.println("[CRITICAL] Mutex taken during init");
        
        // Initialize display state
        digitalWrite(OE_PIN, HIGH); // Ensure display is off during init
        memset(displayBuffer, CHAR_BLANK, DISPLAY_COUNT);
        memset(dpBuffer, 0, DISPLAY_COUNT);
        displayValid = true;
        
        xSemaphoreGive(displayMutex);
        
        // Enable display output
        digitalWrite(OE_PIN, LOW);
        
        // Set initial brightness
        setBrightness(75);  // Default to full brightness
        
        Serial.println("DisplayHandler::init() - Complete");
        return true;  // Return true to indicate successful initialization
    } else {
        Serial.println("[CRITICAL ERROR] Could not take mutex during init!");
        return false;  // Return false if initialization failed
    }
}

void DisplayHandler::updateDisplay() {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint8_t patterns[DISPLAY_COUNT];
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            patterns[i] = SEGMENT_MAP[displayBuffer[i]];
            if (dpBuffer[i]) {
                patterns[i] &= 0x7F;  // Set decimal point (active low for common anode)
            }
        }
        sr.setAll(patterns);
        xSemaphoreGive(displayMutex);
    }
}

void DisplayHandler::setDigit(uint8_t position, uint8_t value, bool dp) {
    if (position < DISPLAY_COUNT && value < sizeof(SEGMENT_MAP)/sizeof(SEGMENT_MAP[0])) {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            displayBuffer[position] = value;
            dpBuffer[position] = dp;
            displayValid = true;
            xSemaphoreGive(displayMutex);
        }
    }
}

void DisplayHandler::update() {
    unsigned long now = millis();
    
    if (now - modeStartTime >= MODE_DURATIONS[static_cast<int>(currentMode)]) {
        nextMode();
        modeStartTime = now;
    }
    
    if (now - lastUpdate >= 100) { // 100ms refresh rate
        if (displayValid) {
            updateDisplay();
        }
        lastUpdate = now;
    }
}

void DisplayHandler::showTime(int hours, int minutes) {
    static bool colonState = true;
    static unsigned long lastToggle = 0;
    unsigned long currentMillis = millis();
    
    // Toggle colon state every 500ms
    if (currentMillis - lastToggle >= 500) {
        colonState = !colonState;
        lastToggle = currentMillis;
    }
    
    // Convert time digits to correct character indices
    setDigit(0, CHAR_0 + (hours / 10));
    setDigit(1, CHAR_0 + (hours % 10), colonState);
    setDigit(2, CHAR_0 + (minutes / 10));
    setDigit(3, CHAR_0 + (minutes % 10));
}

void DisplayHandler::showDate(int day, int month) {
    setDigit(0, CHAR_0 + (day / 10));
    setDigit(1, CHAR_0 + (day % 10), true);
    setDigit(2, CHAR_0 + (month / 10));
    setDigit(3, CHAR_0 + (month % 10));
}

void DisplayHandler::showTemperature(float temp) {
    if (temp < -9.9 || temp > 99.9) {
        clear();
        return;
    }
    
    int whole = abs((int)temp);
    int decimal = abs((int)(temp * 10) % 10);
    
    if (temp < 0) {
        setDigit(0, CHAR_MINUS);
        setDigit(1, CHAR_0 + (whole % 10), true);
        setDigit(2, CHAR_0 + decimal);
        setDigit(3, CHAR_C);
    } else {
        setDigit(0, CHAR_0 + (whole / 10));
        setDigit(1, CHAR_0 + (whole % 10), true);
        setDigit(2, CHAR_0 + decimal);
        setDigit(3, CHAR_C);
    }
}

void DisplayHandler::showHumidity(float humidity) {
    if (humidity < 0 || humidity > 99.9) {
        clear();
        return;
    }
    
    int whole = (int)humidity;
    int decimal = (int)(humidity * 10) % 10;
    
    setDigit(0, CHAR_0 + (whole / 10));
    setDigit(1, CHAR_0 + (whole % 10), true);
    setDigit(2, CHAR_0 + decimal);
    setDigit(3, CHAR_h);
}

void DisplayHandler::showPressure(float pressure) {
    // Display pressure in hPa, rounded to nearest whole number
    int hpa = (int)(pressure + 0.5);
    
    if (hpa > 9999) hpa = 9999;
    
    setDigit(0, CHAR_0 + ((hpa / 1000) % 10));
    setDigit(1, CHAR_0 + ((hpa / 100) % 10));
    setDigit(2, CHAR_0 + ((hpa / 10) % 10));
    setDigit(3, CHAR_0 + (hpa % 10));
}

void DisplayHandler::showRemoteTemp(float temp) {
    /* Serial.println("Showing remote temp: " + String(temp, 2));  // Added precision
       Serial.printf("Raw temp value: %f\n", temp);  // Log raw value
       Serial.printf("Temp comparison: %f <= -40: %s\n", temp, temp <= -40 ? "true" : "false");
    Serial.printf("Temp comparison: %f >= 140: %s\n", temp, temp >= 140 ? "true" : "false");
     */
    // Don't show invalid temperatures
    if (temp <= -40 || temp >= 140) {
        Serial.printf("Invalid remote temp: %.2f, showing error\n", temp);
        setDigit(0, CHAR_r);
        setDigit(1, CHAR_MINUS);
        setDigit(2, CHAR_MINUS);
        setDigit(3, CHAR_MINUS);
        return;
    }

    int wholePart = abs((int)temp);
    int decimalPart = abs((int)(temp * 10) % 10);

    setDigit(0, CHAR_r);  // 'r' for remote
    if (temp < 0) {
        setDigit(1, CHAR_MINUS);
        setDigit(2, CHAR_0 + (wholePart % 10), true);  // Decimal point
        setDigit(3, CHAR_0 + decimalPart);
    } else {
        setDigit(1, CHAR_0 + (wholePart / 10));
        setDigit(2, CHAR_0 + (wholePart % 10), true);  // Decimal point
        setDigit(3, CHAR_0 + decimalPart);
    }
    
    /* Serial.printf("Remote temp display: %s%d.%d (%.1f°C)\n", 
                 temp < 0 ? "-" : "",
                 wholePart,
                 decimalPart,
                 temp);
    */
}

void DisplayHandler::setMode(DisplayMode mode) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentMode = mode;
        modeStartTime = millis();
        xSemaphoreGive(displayMutex);
    }
}

void DisplayHandler::nextMode() {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentMode = static_cast<DisplayMode>((static_cast<int>(currentMode) + 1) % 6);
        modeStartTime = millis();
        xSemaphoreGive(displayMutex);
    }
}

void DisplayHandler::clear() {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(displayBuffer, CHAR_BLANK, DISPLAY_COUNT);
        memset(dpBuffer, 0, DISPLAY_COUNT);
        displayValid = true;
        xSemaphoreGive(displayMutex);
    }
}

void DisplayHandler::test() {
    Serial.println("Running display test sequence...");
    
    // Test counting
    Serial.println("Testing digits 0-9");
    for (int digit = 0; digit <= 9; digit++) {
        for (int pos = 0; pos < DISPLAY_COUNT; pos++) {
            setDigit(pos, CHAR_0 + digit, false);
        }
        setBrightness(25*digit);
        updateDisplay();
        delay(500);
    }
    
    clear();
    updateDisplay();
    Serial.println("Display test complete");
}

void DisplayHandler::setDisplayPreferences(const DisplayPreferences& prefs) {
    // First test if mutex exists
    if (!displayMutex) {
        Serial.println("[CRITICAL] Display mutex is null in setDisplayPreferences");
        return;
    }

    // Try to take mutex multiple times
    for (uint8_t attempts = 0; attempts < MAX_MUTEX_ATTEMPTS; attempts++) {
        if (xSemaphoreTake(displayMutex, MUTEX_TIMEOUT) == pdTRUE) {
            displayPreferences = prefs;
            xSemaphoreGive(displayMutex);
            
            // Get current time to determine if we're in night mode
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                Serial.printf("[PREFS] Applying new brightness settings at hour %d\n", 
                            timeinfo.tm_hour);
                applyNightModeBrightness(timeinfo.tm_hour);
            } else {
                Serial.println("[PREFS] Time not available, using day brightness");
                uint8_t dayBright = map(displayPreferences.dayBrightness, 1, 75, 1, 100);
                setBrightness(dayBright);
            }
            
            return;
        }
        // Wait before next attempt
        vTaskDelay(MUTEX_WAIT);
    }
    
    Serial.println("[CRITICAL] Failed to acquire mutex in setDisplayPreferences after max attempts");
}

bool DisplayHandler::setBrightness(uint8_t brightness) {
    if (!displayMutex) {
        Serial.println("[CRITICAL] Display mutex is null in setBrightness");
        return false;
    }

    bool mutexTaken = false;
    
    // Try multiple times to acquire mutex
    for (uint8_t attempts = 0; attempts < MAX_MUTEX_ATTEMPTS; attempts++) {
        if (xSemaphoreTake(displayMutex, MUTEX_TIMEOUT) == pdTRUE) {
            mutexTaken = true;
            break;
        }
        vTaskDelay(MUTEX_WAIT);
    }
    
    if (!mutexTaken) {
        Serial.println("[CRITICAL] Failed to acquire mutex in setBrightness");
        return false;
    }
    
    // Constrain brightness to valid percentage
    uint8_t constrainedBrightness = constrain(brightness, 1, 100);
    
    // For common anode: 245 = dimmest (1%), 64 = brightest (100%)
    uint8_t pwmValue = map(constrainedBrightness, 1, 100, 245, 64);
    
    pinMode(OE_PIN, OUTPUT);
    analogWrite(OE_PIN, pwmValue);
    currentBrightness = constrainedBrightness;
    
    xSemaphoreGive(displayMutex);
    return true;
}

void DisplayHandler::applyNightModeBrightness(int currentHour) {
    Serial.printf("[NIGHT MODE] Applying night mode at hour %d\n", currentHour);
    Serial.printf("[NIGHT MODE] Night mode enabled: %s\n", 
                  displayPreferences.nightModeDimmingEnabled ? "Yes" : "No");

    if (!displayPreferences.nightModeDimmingEnabled) {
        uint8_t dayBright = map(displayPreferences.dayBrightness, 1, 75, 1, 100);
        Serial.printf("[NIGHT MODE] Night mode disabled, using day brightness: %d%%\n", 
                     dayBright);
        setBrightness(dayBright);
        return;
    }

    bool isNightTime;
    if (displayPreferences.nightStartHour < displayPreferences.nightEndHour) {
        // Simple case (e.g., 22:00 to 06:00)
        isNightTime = (currentHour >= displayPreferences.nightStartHour && 
                      currentHour < displayPreferences.nightEndHour);
    } else {
        // Crossing midnight (e.g., 22:00 to 06:00 next day)
        isNightTime = (currentHour >= displayPreferences.nightStartHour || 
                      currentHour < displayPreferences.nightEndHour);
    }

    // Map stored brightness (1-75) to percentage (1-100)
    uint8_t targetBrightness;
    if (isNightTime) {
        targetBrightness = map(displayPreferences.nightBrightness, 1, 75, 1, 100);
        Serial.printf("[NIGHT MODE] Night mode active, using night brightness: %d%%\n", 
                     targetBrightness);
    } else {
        targetBrightness = map(displayPreferences.dayBrightness, 1, 75, 1, 100);
        Serial.printf("[NIGHT MODE] Day mode active, using day brightness: %d%%\n", 
                     targetBrightness);
    }
    
    Serial.printf("[NIGHT MODE] Setting brightness to: %d%%\n", targetBrightness);
    setBrightness(targetBrightness);
}
