#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "DisplayHandler.h"

// System Configuration
#define FIRMWARE_VERSION "NTPClock_v1.5.5"

// WiFi Portal Configuration
#define WIFI_SETUP_AP_NAME "Chaotivolt_ntp_CLOCK1"
#define WIFI_SETUP_PASSWORD "fabuleux"
#define WIFI_PORTAL_TIMEOUT 300000  // 5 minutes in milliseconds
#define WIFI_CONNECTION_TIMEOUT 30000  // 30 seconds in milliseconds
#define WIFI_MAX_RETRIES 5

// MQTT Configuration
#define MQTT_CLIENT_ID "ablutionoracle1"
#define MQTT_TOPIC_AUX_DISPLAY "sensor"
#define MQTT_TOPIC_RELAY "relay"
#define MQTT_BROKER "homeassistant.local"
#define MQTT_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASSWORD "password"
#define MQTT_SENSORHUB_TOPIC "sensorhub1"

#define MQTT_TOPIC_STATUS "status"
#define MQTT_QOS 1

#define BASE_MDNS_NAME "chaoticvolt"
// mDNS Configuration
#define MDNS_HOSTNAME "chaoticvolt"



// Display Configuration
#define DISPLAY_COUNT 4
#define DISPLAY_TIME_DURATION 8000    // 6 seconds
#define DISPLAY_DATE_DURATION 2000    // 2 seconds
#define DISPLAY_TEMP_DURATION 2000    // 2 seconds
#define DISPLAY_HUM_DURATION 2000     // 2 seconds
#define DISPLAY_PRES_DURATION 2000    // 2 seconds
#define DISPLAY_REMOTE_DURATION 3000  // 2 seconds

// I2C Configuration (BME280)
#define I2C_SDA 21
#define I2C_SCL 22

// Only define BME280_ADDRESS if not already defined by the library
#ifndef BME280_ADDRESS
#define BME280_ADDRESS 0x76
#endif


// API configuration
#define API_SERVER_URL "http://sensorhub.local"
#define API_USERNAME "admin"
#define API_PASSWORD "admin"
#define API_LOGIN_ENDPOINT "/api/login"
#define API_SENSORS_ENDPOINT "/api/sensors"
#define API_RELAY_ENDPOINT "/api/relay"
#define API_TOKEN_REFRESH_INTERVAL (12UL * 60UL * 60UL * 1000UL)  // 12 hours

// Sensorhub aliases - use the API definitions
#define SENSORHUB_URL "sensorhub.local"
#define SENSORHUB_AUTH_ENDPOINT API_LOGIN_ENDPOINT
#define SENSORHUB_RELAY_ENDPOINT API_RELAY_ENDPOINT
#define SENSOR_API_ENDPOINT API_SENSORS_ENDPOINT

// Token refresh interval
#define TOKEN_REFRESH_INTERVAL API_TOKEN_REFRESH_INTERVAL

// Override MQTT_KEEPALIVE only if we want a different value than the library default
#undef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 60

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// Task Configuration
#define STACK_SIZE_DISPLAY 8192
#define STACK_SIZE_SENSOR 8192
#define STACK_SIZE_NETWORK 16384
#define STACK_SIZE_WATCHDOG 4096

#define PRIORITY_DISPLAY 2
#define PRIORITY_SENSOR 1
#define PRIORITY_NETWORK 1
#define PRIORITY_WATCHDOG 3

// Watchdog Configuration
#define WATCHDOG_TIMEOUT 30000  // 30 seconds

// Sensor Update Intervals
#define BME280_UPDATE_INTERVAL 30000  // 30 seconds
#define DISPLAY_UPDATE_INTERVAL 100    // 100 ms
#define MQTT_PUBLISH_INTERVAL 60000    // 60 seconds
 
// Display PIN Configuration
#define DATA_PIN 26
#define CLOCK_PIN 32
#define LATCH_PIN 33
#define OE_PIN 25

// Display configuration
#define DISPLAY_COUNT 4    // Number of 7-segment displays




