#ifndef DEBUG_H
#define DEBUG_H

// Debug macros
#ifdef DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_BEGIN(x) Serial.begin(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_BEGIN(x)
#endif

#endif // DEBUG_H