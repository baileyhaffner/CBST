#pragma once
// =============================================================
//  imu_manager.h
//  Wraps LSM6DSOX init, I2C scan, and per-sample reads.
//  Builds on your existing proven init logic.
// =============================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

static const int IMU_SDA = 8;
static const int IMU_SCL = 9;

static Adafruit_LSM6DSOX _imu;
static uint8_t           _imuAddress = 0;

// Scan I2C bus and return first address found matching 0x6A or 0x6B.
// Returns 0 if neither found.
static uint8_t _scanForIMU() {
    Serial.println("[IMU] Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[IMU] Found device at 0x%02X\n", addr);
            if (addr == 0x6A || addr == 0x6B) return addr;
        }
    }
    return 0;
}

// Attempt init at both addresses. Fallback matches your original logic.
static bool _tryInit() {
    uint8_t preferred = _scanForIMU();

    // Try scanned address first, then fallback to both
    uint8_t order[] = { preferred, 0x6A, 0x6B };
    for (uint8_t addr : order) {
        if (addr == 0 || addr == _imuAddress) continue;  // skip zero / already tried
        if (_imu.begin_I2C(addr)) {
            _imuAddress = addr;
            return true;
        }
    }
    return false;
}

// Initialise I2C bus and IMU. Returns true on success.
bool imuInit() {
    Wire.begin(IMU_SDA, IMU_SCL);
    Wire.setClock(100000);

    if (!_tryInit()) {
        Serial.println("[IMU] Could not initialise LSM6DSOX at 0x6A or 0x6B.");
        return false;
    }

    _imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    _imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
    _imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    _imu.setGyroDataRate(LSM6DS_RATE_104_HZ);

    Serial.printf("[IMU] LSM6DSOX ready at 0x%02X\n", _imuAddress);
    return true;
}

// Struct for one IMU sample
struct ImuSample {
    float ax, ay, az;   // m/s²
    float gx, gy, gz;   // rad/s
    float temp;          // °C
};

// Read one sample from the IMU.
ImuSample imuRead() {
    sensors_event_t a, g, t;
    _imu.getEvent(&a, &g, &t);
    return {
        a.acceleration.x, a.acceleration.y, a.acceleration.z,
        g.gyro.x,         g.gyro.y,         g.gyro.z,
        t.temperature
    };
}

// Format a sample as a CSV row string (no newline)
String imuToCSV(const ImuSample& s) {
    char buf[96];
    snprintf(buf, sizeof(buf),
             "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f",
             s.ax, s.ay, s.az,
             s.gx, s.gy, s.gz,
             s.temp);
    return String(buf);
}