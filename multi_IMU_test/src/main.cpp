#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>
#include "wifi_manager.h"

static constexpr int SDA_PIN    = 8;
static constexpr int SCL_PIN    = 9;
static constexpr int BUTTON_PIN = 41;

static constexpr uint32_t IMU_READ_INTERVAL_MS  = 50;
static constexpr uint32_t IMU_IDLE_INTERVAL_MS  = 10;
static constexpr uint32_t IMU_ERROR_INTERVAL_MS = 1000;

// Two IMU objects, one for each address
Adafruit_LSM6DSOX imu6A;
Adafruit_LSM6DSOX imu6B;

bool found6A = false;
bool found6B = false;
bool imu6AReady = false;
bool imu6BReady = false;

// scans for the addresses we want - also sets the flags
void scanI2C() {
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

void setupSingleIMUConfig(Adafruit_LSM6DSOX &imu) {
  imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
  imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
  imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
}

bool initIMUs() {
  Serial.println();
  Serial.println("Trying to initialise LSM6DSOX sensors");

  // Try 0x6A
  if (found6A) {
    if (imu6A.begin_I2C(0x6A)) {
      setupSingleIMUConfig(imu6A);
      imu6AReady = true;
      Serial.println("LSM6DSOX initialised at 0x6A");
    } else {
      Serial.println("Could not initialise LSM6DSOX at 0x6A");
    }
  }

  // Try 0x6B
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

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("ESP32-S3 dual LSM6DSOX I2C starting");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Setup button
  pinMode(BUTTON_PIN, INPUT);

  scanI2C();

  if (!initIMUs()) {
    Serial.println("Could not initialise any LSM6DSOX sensors.");
    Serial.println("Expected IMUs at 0x6A and/or 0x6B.");
    Serial.println("If you only see 0x36, that is likely another onboard device, not the IMUs.");
    return;
  }

  wifiInit();

  // CSV header
  Serial.println("imu,ax,ay,az,gx,gy,gz,temp");
}

void loop() {
    wifiHandle();

    if (!imu6AReady && !imu6BReady) {
        static uint32_t lastErrorLog = 0;
        if (millis() - lastErrorLog >= IMU_ERROR_INTERVAL_MS) {
            lastErrorLog = millis();
            Serial.printf("[Main] No IMUs ready\n");
        }
        return;
    }

    static uint32_t lastRead = 0;
    int buttonState = digitalRead(BUTTON_PIN);
    uint32_t interval = (buttonState == HIGH) ? IMU_READ_INTERVAL_MS : IMU_IDLE_INTERVAL_MS;

    if (millis() - lastRead >= interval) {
        lastRead = millis();

        if (imu6AReady) {
            sensors_event_t accel, gyro, temp;
            imu6A.getEvent(&accel, &gyro, &temp);
            wifiUpdateIMU("IMU_0x6A",
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
            wifiUpdateIMU("IMU_0x6B",
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
}
