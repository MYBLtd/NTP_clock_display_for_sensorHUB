#pragma once
#include "GlobalState.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// In GlobalDefinitions.h
extern GlobalState* g_state;
extern QueueHandle_t sensorQueue;
extern QueueHandle_t displayQueue;
extern TaskHandle_t displayTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t networkTaskHandle;
extern TaskHandle_t watchdogTaskHandle;