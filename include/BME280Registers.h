/**
 * BME280Registers.h
 * 
 * This file defines register addresses for the BME280 sensor.
 * Filter coefficients and timing values are imported from the official driver.
 */

 #pragma once

 #ifndef BME280_REGISTERS_H
 #define BME280_REGISTERS_H
 
 #include "bme280_defs.h"  // Include the official definitions
 
 // BME280 Register Addresses
 #define BME280_CHIP_ID_ADDR          UINT8_C(0xD0)
 #define BME280_RESET_ADDR            UINT8_C(0xE0)
 #define BME280_CTRL_HUM_ADDR         UINT8_C(0xF2)
 #define BME280_STATUS_ADDR           UINT8_C(0xF3)
 #define BME280_CTRL_MEAS_ADDR        UINT8_C(0xF4)
 #define BME280_CONFIG_ADDR           UINT8_C(0xF5)
 #define BME280_PRESS_MSB_ADDR        UINT8_C(0xF7)
 #define BME280_PRESS_LSB_ADDR        UINT8_C(0xF8)
 #define BME280_PRESS_XLSB_ADDR       UINT8_C(0xF9)
 #define BME280_TEMP_MSB_ADDR         UINT8_C(0xFA)
 #define BME280_TEMP_LSB_ADDR         UINT8_C(0xFB)
 #define BME280_TEMP_XLSB_ADDR        UINT8_C(0xFC)
 #define BME280_HUM_MSB_ADDR          UINT8_C(0xFD)
 #define BME280_HUM_LSB_ADDR          UINT8_C(0xFE)
 
 // BME280 Chip ID
 #define BME280_CHIP_ID               UINT8_C(0x60)
 
 // Power Modes
 #define BME280_SLEEP_MODE            UINT8_C(0x00)
 #define BME280_FORCED_MODE           UINT8_C(0x01)
 #define BME280_NORMAL_MODE           UINT8_C(0x03)
 
 // Oversampling definitions for temperature
 #define BME280_NO_OVERSAMPLING       UINT8_C(0x00)
 #define BME280_OVERSAMPLING_1X       UINT8_C(0x01)
 #define BME280_OVERSAMPLING_2X       UINT8_C(0x02)
 #define BME280_OVERSAMPLING_4X       UINT8_C(0x03)
 #define BME280_OVERSAMPLING_8X       UINT8_C(0x04)
 #define BME280_OVERSAMPLING_16X      UINT8_C(0x05)
 
 #endif // BME280_REGISTERS_H