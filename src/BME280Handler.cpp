/**
 * BME280Handler.cpp
 * 
 * Implementation of the BME280Handler class using direct register access
 * and raw data compensation algorithms from the BME280 datasheet.
 */

#include "BME280Handler.h"
#include "BME280Registers.h"

// Store t_fine value globally since it's needed for pressure and humidity compensation
static int32_t t_fine;

BME280Handler::BME280Handler()
    : deviceAddress(0x76)
    , temperature(BME280_INVALID_TEMP)
    , humidity(BME280_INVALID_HUM)
    , pressure(BME280_INVALID_PRES)
    , sensorValid(false)
    , lastReadTime(0)
{
    memset(&calibData, 0, sizeof(calibData));
}

bool BME280Handler::init() {
    Serial.println("Starting BME280 initialization...");
    
    // Debug I2C detection
    Wire.beginTransmission(deviceAddress);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.printf("I2C device not found at address 0x%02X (error: %d)\n", deviceAddress, error);
        return false;
    }
    
    // Read chip ID with delay
    uint8_t chipId;
    delay(10); // Add small delay before reading
    if (i2cRead(BME280_CHIP_ID_ADDR, &chipId, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to read chip ID");
        return false;
    }
    
    if (chipId != BME280_CHIP_ID) {
        Serial.printf("Invalid chip ID: 0x%02X\n", chipId);
        return false;
    }
    
    // Reset the sensor
    uint8_t reset_cmd = BME280_RESET_CMD;
    if (i2cWrite(BME280_RESET_ADDR, &reset_cmd, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to reset BME280");
        return false;
    }
    delay(10);  // Wait for reset
    
    // Read calibration data
    if (!readCalibrationData()) {
        Serial.println("Failed to read calibration data");
        return false;
    }

    // Configure the sensor
    // Humidity oversampling x1
    uint8_t ctrl_hum = 0x01;
    if (i2cWrite(BME280_CTRL_HUM_ADDR, &ctrl_hum, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to write ctrl_hum");
        return false;
    }

    // Temperature oversampling x2, Pressure oversampling x2, Normal mode
    uint8_t ctrl_meas = 0x6B;  // 0b01101011
    if (i2cWrite(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to write ctrl_meas");
        return false;
    }

    // Config register: Standby 62.5ms, Filter x4
    uint8_t config = 0x30;  // 0b00110000
    if (i2cWrite(BME280_CONFIG_ADDR, &config, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to write config");
        return false;
    }
    
    Serial.println("BME280 initialization successful");
    return true;
}

bool BME280Handler::readCalibrationData() {
    uint8_t buffer[32];
    
    if (i2cRead(0x88, buffer, 6, &deviceAddress) != BME280_OK) {
        return false;
    }
    calibData.dig_t1 = (uint16_t)(buffer[1] << 8 | buffer[0]);
    calibData.dig_t2 = (int16_t)(buffer[3] << 8 | buffer[2]);
    calibData.dig_t3 = (int16_t)(buffer[5] << 8 | buffer[4]);
    
    if (i2cRead(0x8E, buffer, 18, &deviceAddress) != BME280_OK) {
        return false;
    }
    calibData.dig_p1 = (uint16_t)(buffer[1] << 8 | buffer[0]);
    calibData.dig_p2 = (int16_t)(buffer[3] << 8 | buffer[2]);
    calibData.dig_p3 = (int16_t)(buffer[5] << 8 | buffer[4]);
    calibData.dig_p4 = (int16_t)(buffer[7] << 8 | buffer[6]);
    calibData.dig_p5 = (int16_t)(buffer[9] << 8 | buffer[8]);
    calibData.dig_p6 = (int16_t)(buffer[11] << 8 | buffer[10]);
    calibData.dig_p7 = (int16_t)(buffer[13] << 8 | buffer[12]);
    calibData.dig_p8 = (int16_t)(buffer[15] << 8 | buffer[14]);
    calibData.dig_p9 = (int16_t)(buffer[17] << 8 | buffer[16]);

    uint8_t h1;
    if (i2cRead(0xA1, &h1, 1, &deviceAddress) != BME280_OK) {
        return false;
    }
    calibData.dig_h1 = h1;
    
    if (i2cRead(0xE1, buffer, 7, &deviceAddress) != BME280_OK) {
        return false;
    }
    
    calibData.dig_h2 = (int16_t)(buffer[1] << 8 | buffer[0]);
    calibData.dig_h3 = buffer[2];
    calibData.dig_h4 = (int16_t)((buffer[3] << 4) | (buffer[4] & 0x0F));
    calibData.dig_h5 = (int16_t)((buffer[5] << 4) | (buffer[4] >> 4));
    calibData.dig_h6 = (int8_t)buffer[6];
    
    return true;
}

bool BME280Handler::takeMeasurement() {
    esp_task_wdt_reset();
    
    // Read current control settings
    uint8_t ctrl_meas;
    if (i2cRead(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to read ctrl_meas");
        return false;
    }
    
    // Force measurement
    ctrl_meas = (ctrl_meas & 0xFC) | 0x01;
    if (i2cWrite(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to write ctrl_meas");
        return false;
    }
    
    // Wait for measurement
    delay(10);
    
    // Read raw values
    uint8_t buffer[8];
    if (i2cRead(BME280_PRESS_MSB_ADDR, buffer, 8, &deviceAddress) != BME280_OK) {
        Serial.println("Failed to read measurements");
        return false;
    }
    
    // Parse raw values
    int32_t rawPressure = ((uint32_t)buffer[0] << 12) | ((uint32_t)buffer[1] << 4) | ((uint32_t)buffer[2] >> 4);
    Serial.print("Raw pressure: 0x");
    Serial.println(rawPressure, HEX);
    
    int32_t rawTemp = ((uint32_t)buffer[3] << 12) | ((uint32_t)buffer[4] << 4) | ((uint32_t)buffer[5] >> 4);
    int32_t rawHumidity = ((uint32_t)buffer[6] << 8) | (uint32_t)buffer[7];
    
    // Calculate temperature first
    temperature = compensateTemperature(rawTemp);
    pressure = compensatePressure(rawPressure);
    humidity = compensateHumidity(rawHumidity);

    // Print measurements in smaller chunks to use less stack space
    Serial.print("Measurements: T=");
    Serial.print(temperature);
    Serial.print("°C, H=");
    Serial.print(humidity);
    Serial.print("%, P=");
    Serial.print(pressure);
    Serial.println("hPa");
    
    sensorValid = true;
    lastReadTime = millis();
    return true;
}

void BME280Handler::processRawMeasurements(uint8_t* buffer) {
    // Parse the raw measurement data
    uint32_t pressure_msb = (uint32_t)buffer[0] << 12;
    uint32_t pressure_lsb = (uint32_t)buffer[1] << 4;
    uint32_t pressure_xlsb = (uint32_t)buffer[2] >> 4;
    rawPressure = pressure_msb | pressure_lsb | pressure_xlsb;

    uint32_t temp_msb = (uint32_t)buffer[3] << 12;
    uint32_t temp_lsb = (uint32_t)buffer[4] << 4;
    uint32_t temp_xlsb = (uint32_t)buffer[5] >> 4;
    rawTemperature = temp_msb | temp_lsb | temp_xlsb;

    uint32_t hum_msb = (uint32_t)buffer[6] << 8;
    uint32_t hum_lsb = (uint32_t)buffer[7];
    rawHumidity = hum_msb | hum_lsb;
}

float BME280Handler::compensateTemperature(int32_t adc_T) {
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)calibData.dig_t1 << 1))) *
                    ((int32_t)calibData.dig_t2)) >> 11;
                    
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)calibData.dig_t1)) *
                      ((adc_T >> 4) - ((int32_t)calibData.dig_t1))) >> 12) *
                    ((int32_t)calibData.dig_t3)) >> 14;
                    
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

float BME280Handler::compensatePressure(int32_t adc_P) {
    int64_t var1, var2, p;
    
    // Calculate var1
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calibData.dig_p6;
    var2 = var2 + ((var1 * (int64_t)calibData.dig_p5) << 17);
    var2 = var2 + (((int64_t)calibData.dig_p4) << 35);
    var1 = ((var1 * var1 * (int64_t)calibData.dig_p3) >> 8) + ((var1 * (int64_t)calibData.dig_p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calibData.dig_p1) >> 33;
    
    if (var1 == 0) {
        return BME280_INVALID_PRES;  // Avoid division by zero
    }
    
    // Calculate pressure
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calibData.dig_p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calibData.dig_p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calibData.dig_p7) << 4);
    
    // Convert to hPa (p is in Pa units)
    float pressure = ((float)p / 256.0f) / 100.0f;
    
    // Validate pressure range
    if (pressure < BME280_PRES_MIN || pressure > BME280_PRES_MAX) {
        Serial.printf("Invalid pressure calculated: %.1f hPa\n", pressure);
        return BME280_INVALID_PRES;
    }
    
    return pressure;
}

float BME280Handler::compensateHumidity(int32_t adc_H) {
    int32_t v_x1_u32r;
    
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calibData.dig_h4) << 20) -
                    (((int32_t)calibData.dig_h5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)calibData.dig_h6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)calibData.dig_h3)) >> 11) +
                       ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)calibData.dig_h2) +
                   8192) >> 14));
                   
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)calibData.dig_h1)) >> 4));
    
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    
    return (float)(v_x1_u32r >> 12) / 1024.0f;
}

bool BME280Handler::validateReadings(float temp, float hum, float pres) {
    if (temp < BME280_TEMP_MIN || temp > BME280_TEMP_MAX) {
        Serial.printf("Invalid temperature reading: %.2f°C\n", temp);
        return false;
    }
    
    if (hum < BME280_HUM_MIN || hum > BME280_HUM_MAX) {
        Serial.printf("Invalid humidity reading: %.1f%%\n", hum);
        return false;
    }
    
    if (pres < BME280_PRES_MIN || pres > BME280_PRES_MAX) {
        Serial.printf("Invalid pressure reading: %.1fhPa\n", pres);
        return false;
    }
    
    return true;
}

void BME280Handler::setupSensorSettings() {
    // Set humidity oversampling to 1x
    uint8_t ctrl_hum = BME280_OVERSAMPLING_1X;
    i2cWrite(BME280_CTRL_HUM_ADDR, &ctrl_hum, 1, &deviceAddress);
    
    // Set temperature and pressure oversampling to 16x
    uint8_t ctrl_meas = (BME280_OVERSAMPLING_16X << BME280_CTRL_TEMP_POS) |
                        (BME280_OVERSAMPLING_16X << BME280_CTRL_PRESS_POS) |
                        BME280_NORMAL_MODE;
    i2cWrite(BME280_CTRL_MEAS_ADDR, &ctrl_meas, 1, &deviceAddress);
    
    // Set IIR filter coefficient to 16
    uint8_t config = (BME280_FILTER_COEFF_16 << 2) | BME280_STANDBY_TIME_1000_MS;
    i2cWrite(BME280_CONFIG_ADDR, &config, 1, &deviceAddress);
}

int8_t BME280Handler::i2cRead(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t dev_addr = *(uint8_t*)intf_ptr;
    
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    Wire.endTransmission(false);
    
    Wire.requestFrom(dev_addr, len);
    if(Wire.available() != len) {
        return -1;
    }
    
    for(uint32_t i = 0; i < len; i++) {
        reg_data[i] = Wire.read();
    }
    
    return 0;
}

int8_t BME280Handler::i2cWrite(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t dev_addr = *(uint8_t*)intf_ptr;
    
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    Wire.write(reg_data, len);
    if(Wire.endTransmission() != 0) {
        return -1;
    }
    
    return 0;
}

bool BME280Handler::initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Try both possible I2C addresses
    if(tryAddress(BME280_I2C_ADDR_PRIM)) {
        deviceAddress = BME280_I2C_ADDR_PRIM;
        return true;
    }
    
    if(tryAddress(BME280_I2C_ADDR_SEC)) {
        deviceAddress = BME280_I2C_ADDR_SEC;
        return true;
    }
    
    return false;
}

bool BME280Handler::tryAddress(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

// Implement watchdog task
void watchdogTask(void *parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for(;;) {
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}
