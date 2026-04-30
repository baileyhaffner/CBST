#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

namespace ImuManager {

static constexpr int SDA_PIN    = 8;
static constexpr int SCL_PIN    = 9;
static constexpr int BUTTON_PIN = 41;

static constexpr uint32_t IMU_READ_INTERVAL_MS  = 50;
static constexpr uint32_t IMU_IDLE_INTERVAL_MS  = 10;
static constexpr uint32_t IMU_ERROR_INTERVAL_MS = 1000;

// Two IMU objects, one for each address
static Adafruit_LSM6DSOX imu6A;
static Adafruit_LSM6DSOX imu6B;

static bool found6A = false;
static bool found6B = false;
static bool imu6AReady = false;
static bool imu6BReady = false;

inline void scanI2C() {
    found6A = false;
    found6B = false;

    Serial.println();
    Serial.println("Scanning I2C bus");

    bool foundAny = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            foundAny = true;

            Serial.print("Found I2C device at 0x");
            if (addr < 16) {
                Serial.print('0');
            }
            Serial.println(addr, HEX);

            if (addr == 0x6A) {
                found6A = true;
            } else if (addr == 0x6B) {
                found6B = true;
            }
        } else if (error == 4) {
            Serial.print("Error at 0x");
            if (addr < 16) {
                Serial.print('0');
            }
            Serial.println(addr, HEX);
        }
    }

    if (!foundAny) {
        Serial.println("No I2C devices found");
    }
}

inline void setupSingleIMUConfig(Adafruit_LSM6DSOX& imu) {
    imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
}

inline bool initIMUs() {
    imu6AReady = false;
    imu6BReady = false;

    Serial.println();
    Serial.println("Trying to initialise LSM6DSOX sensors");

    if (found6A) {
        if (imu6A.begin_I2C(0x6A)) {
            setupSingleIMUConfig(imu6A);
            imu6AReady = true;
            Serial.println("LSM6DSOX initialised at 0x6A");
        } else {
            Serial.println("Could not initialise LSM6DSOX at 0x6A");
        }
    }

    if (found6B) {
        if (imu6B.begin_I2C(0x6B)) {
            setupSingleIMUConfig(imu6B);
            imu6BReady = true;
            Serial.println("LSM6DSOX initialised at 0x6B");
        } else {
            Serial.println("Could not initialise LSM6DSOX at 0x6B");
        }
    }

    return imu6AReady || imu6BReady;
}

inline void beginBusAndButton() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    pinMode(BUTTON_PIN, INPUT);
}

inline bool anyReady() {
    return imu6AReady || imu6BReady;
}

inline int readButton() {
    return digitalRead(BUTTON_PIN);
}

inline uint32_t getIntervalMs(int buttonState) {
    return (buttonState == HIGH) ? IMU_READ_INTERVAL_MS : IMU_IDLE_INTERVAL_MS;
}

template <typename WifiUpdateFn>
inline void serviceAndPublish(WifiUpdateFn wifiUpdateFn) {
    static uint32_t lastRead = 0;

    if (!imu6AReady && !imu6BReady) {
        static uint32_t lastErrorLog = 0;
        if (millis() - lastErrorLog >= IMU_ERROR_INTERVAL_MS) {
            lastErrorLog = millis();
            Serial.printf("[Main] No IMUs ready\n");
        }
        return;
    }

    int buttonState = readButton();
    uint32_t interval = getIntervalMs(buttonState);

    if (millis() - lastRead < interval) {
        return;
    }

    lastRead = millis();

    if (imu6AReady) {
        sensors_event_t accel, gyro, temp;
        imu6A.getEvent(&accel, &gyro, &temp);

        wifiUpdateFn("IMU_0x6A",
                     accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
                     gyro.gyro.x, gyro.gyro.y, gyro.gyro.z,
                     temp.temperature);

        if (buttonState == HIGH) {
            Serial.printf("IMU_0x6A,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f\n",
                          accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
                          gyro.gyro.x, gyro.gyro.y, gyro.gyro.z, temp.temperature);
        }
    }

    if (imu6BReady) {
        sensors_event_t accel, gyro, temp;
        imu6B.getEvent(&accel, &gyro, &temp);

        wifiUpdateFn("IMU_0x6B",
                     accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
                     gyro.gyro.x, gyro.gyro.y, gyro.gyro.z,
                     temp.temperature);

        if (buttonState == HIGH) {
            Serial.printf("IMU_0x6B,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f\n",
                          accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
                          gyro.gyro.x, gyro.gyro.y, gyro.gyro.z, temp.temperature);
        }
    }
}

}  // namespace ImuManager