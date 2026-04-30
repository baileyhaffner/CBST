#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

static constexpr int SDA_PIN    = 8;
static constexpr int SCL_PIN    = 9;
static constexpr int BUTTON_PIN = 41;

static constexpr uint32_t IMU_READ_INTERVAL_MS  = 50;
static constexpr uint32_t IMU_ERROR_INTERVAL_MS = 1000;

Adafruit_LSM6DSOX imu6A;
Adafruit_LSM6DSOX imu6B;

bool found6A = false;
bool found6B = false;
bool imu6AReady = false;
bool imu6BReady = false;

void scanI2C() {
    found6A = false;
    found6B = false;

    Serial.println();
    Serial.println("[IMU] Scanning I2C bus...");

    bool foundAny = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            foundAny = true;

            Serial.print("[IMU] Found I2C device at 0x");
            if (addr < 16) Serial.print('0');
            Serial.println(addr, HEX);

            if (addr == 0x6A) found6A = true;
            if (addr == 0x6B) found6B = true;
        }
    }

    if (!foundAny) {
        Serial.println("[IMU] No I2C devices found.");
    }
}

void setupSingleIMUConfig(Adafruit_LSM6DSOX &imu) {
    imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
}

bool initIMUs() {
    Serial.println();
    Serial.println("[IMU] Trying to initialise LSM6DSOX sensors...");

    if (found6A) {
        if (imu6A.begin_I2C(0x6A)) {
            setupSingleIMUConfig(imu6A);
            imu6AReady = true;
            Serial.println("[IMU] LSM6DSOX initialised at 0x6A.");
        } else {
            Serial.println("[IMU] Could not initialise LSM6DSOX at 0x6A.");
        }
    }

    if (found6B) {
        if (imu6B.begin_I2C(0x6B)) {
            setupSingleIMUConfig(imu6B);
            imu6BReady = true;
            Serial.println("[IMU] LSM6DSOX initialised at 0x6B.");
        } else {
            Serial.println("[IMU] Could not initialise LSM6DSOX at 0x6B.");
        }
    }

    return imu6AReady || imu6BReady;
}

String readIMUToCSVLine(
    const char* imuName,
    Adafruit_LSM6DSOX& imu,
    uint32_t sampleNumber
) {
    sensors_event_t accel, gyro, temp;
    imu.getEvent(&accel, &gyro, &temp);

    String line = "";
    line += sampleNumber;
    line += ",";
    line += millis();
    line += ",";
    line += imuName;
    line += ",";
    line += String(accel.acceleration.x, 4);
    line += ",";
    line += String(accel.acceleration.y, 4);
    line += ",";
    line += String(accel.acceleration.z, 4);
    line += ",";
    line += String(gyro.gyro.x, 4);
    line += ",";
    line += String(gyro.gyro.y, 4);
    line += ",";
    line += String(gyro.gyro.z, 4);
    line += ",";
    line += String(temp.temperature, 2);

    return line;
}