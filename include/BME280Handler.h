/**
 * BME280Handler.h
 * 
 * This class provides a wrapper around the Bosch BME280 sensor driver,
 * offering a clean interface for sensor operations while handling the
 * low-level details of sensor communication.
 */

 #pragma once

 #include <Arduino.h>
 #include <Wire.h>
 #include "bme280.h"
 #include "config.h"
 #include <esp_task_wdt.h>
 #include <esp_core_dump.h>
 #include <esp_system.h>
 
 // BME280 I2C Address (default)
 #define BME280_I2C_ADDR_PRIM          UINT8_C(0x76)
 #define BME280_I2C_ADDR_SEC           UINT8_C(0x77)
 
 // Reset command (if not defined in the BME280 library)
 #ifndef BME280_RESET_CMD
 #define BME280_RESET_CMD              UINT8_C(0xB6)
 #endif
 
 // Only define constants if they're not already defined in the BME280 library
 #ifndef BME280_OK
 #define BME280_OK                     INT8_C(0)
 #endif
 
 // I2C pin definitions (if not defined in config.h)
 #ifndef I2C_SDA
 #define I2C_SDA 21
 #endif
 
 #ifndef I2C_SCL
 #define I2C_SCL 22
 #endif
 
 // Valid reading ranges based on BME280 datasheet
 constexpr float BME280_TEMP_MIN = -40.0f;    // Minimum operating temperature
 constexpr float BME280_TEMP_MAX = 85.0f;     // Maximum operating temperature
 constexpr float BME280_HUM_MIN = 0.0f;       // Minimum humidity reading
 constexpr float BME280_HUM_MAX = 100.0f;     // Maximum humidity reading
 constexpr float BME280_PRES_MIN = 300.0f;    // Minimum pressure in hPa
 constexpr float BME280_PRES_MAX = 1100.0f;   // Maximum pressure in hPa
 
 class BME280Handler {
     public:
         BME280Handler();
         bool init();
         bool takeMeasurement();
         float getTemperature() const { return temperature; }
         float getHumidity() const { return humidity; }
         float getPressure() const { return pressure; }
         bool isWorking() const { return sensorWorking; }
         
     private:
         // I2C communication helper methods
         int8_t i2cRead(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
         int8_t i2cWrite(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
         bool initI2C();
         bool tryAddress(uint8_t address);
         
         // Calibration and measurement methods
         bool readCalibrationData();
         void processRawMeasurements(uint8_t* buffer);
         float compensateTemperature(int32_t adc_T);
         float compensatePressure(int32_t adc_P);
         float compensateHumidity(int32_t adc_H);
         bool validateReadings(float temp, float hum, float pres);
         void setupSensorSettings();
         
         // Sensor state
         uint8_t deviceAddress;
         SemaphoreHandle_t dataMutex;
         float temperature;
         float humidity;
         float pressure;
         bool sensorWorking;
         bool sensorValid;
         unsigned long lastReadTime;
         
         // Calibration data - using the struct from the BME280 library
         struct bme280_calib_data calibData;
         
         // Raw measurement data
         int32_t rawTemperature;
         int32_t rawPressure;
         int32_t rawHumidity;
 };