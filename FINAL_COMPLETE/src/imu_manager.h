#pragma once

// ================================
// Libraries
// ================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

// ================================
// Select the closest LSM6DSOX hardware data rate
// Do not change this directly
// ================================
static constexpr decltype(LSM6DS_RATE_833_HZ) getIMUDataRateFromSampleRate(uint32_t sampleRateHz) {
    return (sampleRateHz <= 12)  ? LSM6DS_RATE_12_5_HZ :
           (sampleRateHz <= 26)  ? LSM6DS_RATE_26_HZ :
           (sampleRateHz <= 52)  ? LSM6DS_RATE_52_HZ :
           (sampleRateHz <= 104) ? LSM6DS_RATE_104_HZ :
           (sampleRateHz <= 208) ? LSM6DS_RATE_208_HZ :
           (sampleRateHz <= 416) ? LSM6DS_RATE_416_HZ :
                                   LSM6DS_RATE_833_HZ;
}

// ================================
// IMU data storage structure
// ================================
struct IMUData {
    uint32_t timestamp_ms;
    uint32_t sample_number;

    uint8_t imu_id;

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float temp;
};

// ================================
// IMU manager class
// ================================
class IMUManager {
public:
    Adafruit_LSM6DSOX imu6A;
    Adafruit_LSM6DSOX imu6B;

    bool imu6AReady = false;
    bool imu6BReady = false;

    bool begin(uint32_t sampleRateHz) {
        scanI2C();
        initIMUs(sampleRateHz);

        return imu6AReady || imu6BReady;
    }

    bool readIMU(uint8_t address,
                 IMUData &out,
                 uint32_t timestamp,
                 uint32_t sampleNumber) {

        sensors_event_t accel;
        sensors_event_t gyro;
        sensors_event_t temp;

        if (address == 0x6A && imu6AReady) {
            imu6A.getEvent(&accel, &gyro, &temp);
            fillData(out, accel, gyro, temp, 0x6A, timestamp, sampleNumber);
            return true;
        }

        if (address == 0x6B && imu6BReady) {
            imu6B.getEvent(&accel, &gyro, &temp);
            fillData(out, accel, gyro, temp, 0x6B, timestamp, sampleNumber);
            return true;
        }

        return false;
    }

private:
    bool found6A = false;
    bool found6B = false;

    // ================================
    // Scan I2C bus for IMUs
    // ================================
    void scanI2C() {
        found6A = false;
        found6B = false;

        Serial.println();
        Serial.println("Scanning I2C bus...");

        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            uint8_t error = Wire.endTransmission();

            if (error == 0) {
                Serial.print("Found I2C device at 0x");

                if (addr < 16) {
                    Serial.print("0");
                }

                Serial.println(addr, HEX);

                if (addr == 0x6A) {
                    found6A = true;
                }
                else if (addr == 0x6B) {
                    found6B = true;
                }
            }
        }
    }

    // ================================
    // Initialise detected IMUs
    // ================================
    void initIMUs(uint32_t sampleRateHz) {
        Serial.println();
        Serial.println("Initialising LSM6DSOX IMUs...");

        if (found6A) {
            if (imu6A.begin_I2C(0x6A)) {
                configureIMU(imu6A, sampleRateHz);
                imu6AReady = true;
                Serial.println("LSM6DSOX ready at 0x6A");
            }
            else {
                imu6AReady = false;
                Serial.println("Failed to initialise LSM6DSOX at 0x6A");
            }
        }

        if (found6B) {
            if (imu6B.begin_I2C(0x6B)) {
                configureIMU(imu6B, sampleRateHz);
                imu6BReady = true;
                Serial.println("LSM6DSOX ready at 0x6B");
            }
            else {
                imu6BReady = false;
                Serial.println("Failed to initialise LSM6DSOX at 0x6B");
            }
        }
    }

    // ================================
    // Configure IMU range and data rate
    // ================================
    void configureIMU(Adafruit_LSM6DSOX &imu, uint32_t sampleRateHz) {
        imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
        imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);

        auto dataRate = getIMUDataRateFromSampleRate(sampleRateHz);

        imu.setAccelDataRate(dataRate);
        imu.setGyroDataRate(dataRate);
    }

    // ================================
    // Fill IMU data structure
    // ================================
    void fillData(IMUData &d,
                  const sensors_event_t &accel,
                  const sensors_event_t &gyro,
                  const sensors_event_t &temp,
                  uint8_t imuID,
                  uint32_t timestamp,
                  uint32_t sampleNumber) {

        d.timestamp_ms = timestamp;
        d.sample_number = sampleNumber;

        d.imu_id = imuID;

        d.ax = accel.acceleration.x;
        d.ay = accel.acceleration.y;
        d.az = accel.acceleration.z;

        d.gx = gyro.gyro.x;
        d.gy = gyro.gyro.y;
        d.gz = gyro.gyro.z;

        d.temp = temp.temperature;
    }
};