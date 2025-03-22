// GlobalDefinitions.cpp
#include "GlobalDefinitions.h"

// Global variable definitions
GlobalState* g_state = nullptr;
QueueHandle_t sensorQueue = nullptr;
QueueHandle_t displayQueue = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t sensorTaskHandle = nullptr;
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t watchdogTaskHandle = nullptr;