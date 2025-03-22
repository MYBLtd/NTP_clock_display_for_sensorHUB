#pragma once

// Include config.h first to get all definitions
#include "config.h"
#include "SystemDefinitions.h"
#ifndef DISPLAY_COUNT
#define DISPLAY_COUNT 4  // Fallback definition if not defined in config.h
#endif

#include <Arduino.h>
#include <ShiftRegister74HC595.h>
#include <esp_task_wdt.h>
#include "GlobalState.h"

/* Define indices for each character */
// Alphabetic characters (0-16)
#define CHAR_A  0   // 0x88 segments ABCEFG
#define CHAR_b  1   // 0x83 segments CDEFG
#define CHAR_C  2   // 0xC6 segments ADEF
#define CHAR_d  3   // 0xA1 segments BCDEG
#define CHAR_E  4   // 0x86 segments ADEFG
#define CHAR_F  5   // 0x8E segments AEFG
#define CHAR_G  6   // 0x82 segments ACDEFG
#define CHAR_H  7   // 0x89 segments BCEFG
#define CHAR_I  8   // 0xF9 segments BC
#define CHAR_J  9   // 0xF1 segments BCD
#define CHAR_L  10  // 0xC7 segments DEF
#define CHAR_O  11  // 0xC0 segments ABCDEF
#define CHAR_P  12  // 0x8C segments ABEFG
#define CHAR_S  13  // 0x92 segments ACDFG
#define CHAR_U  14  // 0xC1 segments BCDEF
#define CHAR_Y  15  // 0x91 segments BCDFG
#define CHAR_r  16  // 0xAF segments EG

// Numbers (17-26)
#define CHAR_0  17  // 0xC0 segments ABCDEF
#define CHAR_1  18  // 0xF9 segments BC
#define CHAR_2  19  // 0xA4 segments ABDEG
#define CHAR_3  20  // 0xB0 segments ABCDG
#define CHAR_4  21  // 0x99 segments BCFG
#define CHAR_5  22  // 0x92 segments ACDFG
#define CHAR_6  23  // 0x82 segments ACDEFG
#define CHAR_7  24  // 0xF8 segments ABC
#define CHAR_8  25  // 0x80 segments ABCDEFG
#define CHAR_9  26  // 0x90 segments ABCDFG

// Special characters (27-29)
#define CHAR_MINUS 27  // 0xBF segments G
#define CHAR_BLANK 28  // 0xFF no segments
#define CHAR_h     29  // 0x8B segments CEFG
#define CHAR_n  30
#define CHAR_t  31
/* 
   7-Segment Display Bit Mapping (Common Anode):
   -------------------------------------------------------------
   Bit:     7    6    5    4    3    2    1    0
           [DP] [G]  [F]  [E]  [D]  [C]  [B]  [A]

   Note: 0 in a bit position means the segment is ON.
         1 in a bit position means the segment is OFF.
*/

const uint8_t SEGMENT_MAP[] = {
    // Alphabetic characters (0-16)
    0b10001000,  // A: ABCEFG (0)
    0b10000011,  // b: CDEFG (1)
    0b11000110,  // C: ADEF (2)
    0b10100001,  // d: BCDEG (3)
    0b10000110,  // E: ADEFG (4)
    0b10001110,  // F: AEFG (5)
    0b10000010,  // G: ACDEFG (6)
    0b10001001,  // H: BCEFG (7)
    0b11111001,  // I: BC (8)
    0b11110001,  // J: BCD (9)
    0b11000111,  // L: DEF (10)
    0b11000000,  // O: ABCDEF (11)
    0b10001100,  // P: ABEFG (12)
    0b10010010,  // S: ACDFG (13)
    0b11000001,  // U: BCDEF (14)
    0b10010001,  // Y: BCDFG (15)
    0b10101111,  // r: EG (16)
    
    // Numbers (17-26)
    0b11000000,  // 0: ABCDEF (17)
    0b11111001,  // 1: BC (18)
    0b10100100,  // 2: ABDEG (19)
    0b10110000,  // 3: ABCDG (20)
    0b10011001,  // 4: BCFG (21)
    0b10010010,  // 5: ACDFG (22)
    0b10000010,  // 6: ACDEFG (23)
    0b11111000,  // 7: ABC (24)
    0b10000000,  // 8: ABCDEFG (25)
    0b10010000,  // 9: ABCDFG (26)
    
    // Special characters (27-31)
    0b10111111,  // MINUS: G (27)
    0b11111111,  // BLANK: none (28)
    0b10001011,  // h: CEFG (29)
    0b10101011,  // n: CEG (30)
    0b10000111   // t: DEFG (31)
};

class DisplayHandler {
    private:
        // Constants for mutex handling
        static constexpr uint8_t MAX_MUTEX_ATTEMPTS = 5;
        static constexpr TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(100);
        static constexpr TickType_t MUTEX_WAIT = pdMS_TO_TICKS(50);
    
        // Display buffers
        uint8_t displayBuffer[DISPLAY_COUNT];
        uint8_t dpBuffer[DISPLAY_COUNT];
    
        // Add MODE_DURATIONS as static member
        static const unsigned long MODE_DURATIONS[6];
    
        // Existing private members
        ShiftRegister74HC595<4> sr;
        SemaphoreHandle_t displayMutex;
        bool displayValid;
        DisplayMode currentMode;
        unsigned long modeStartTime;
        unsigned long lastUpdate;
        uint8_t currentBrightness;
        DisplayPreferences displayPreferences;
    
        // Add private method declarations
        void updateDisplay();
    
    public:
        DisplayHandler();
        bool init();
        void setDigit(uint8_t position, uint8_t value, bool dp = false);
        void update();
        void setMode(DisplayMode mode);
        void nextMode();
        void clear();
        bool setBrightness(uint8_t brightness);
        
        // Add public method declarations
        void showTime(int hours, int minutes);
        void showDate(int day, int month);
        void showTemperature(float temp);
        void showHumidity(float humidity);
        void showPressure(float pressure);
        void showRemoteTemp(float temp);
        void test();
    
        // Existing public methods
        DisplayMode getCurrentMode() const { return currentMode; }
        void setDisplayPreferences(const DisplayPreferences& prefs);
        const DisplayPreferences& getDisplayPreferences() const { return displayPreferences; }
        void applyNightModeBrightness(int currentHour);
        bool isMutexValid() const { return displayMutex != nullptr; }
    };