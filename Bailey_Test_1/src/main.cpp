#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

static const int SDA_PIN = 8;
static const int SCL_PIN = 9;

Adafruit_LSM6DSOX imu;

bool imuReady = false;
bool found6A = false;
bool found6B = false;
uint8_t imuAddress = 0;

void scanI2C() {
  found6A = false;
  found6B = false;

  Serial.println();
  Serial.println("Scanning I2C bus...");

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
      Serial.print("Unknown error at 0x");
      if (addr < 16) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);
    }
  }

  if (!foundAny) {
    Serial.println("No I2C devices found.");
  }
}

bool initIMU() {
  Serial.println();
  Serial.println("Trying to initialise LSM6DSOX...");

  if (found6A) {
    if (imu.begin_I2C(0x6A)) {
      imuAddress = 0x6A;
      return true;
    }
  }

  if (found6B) {
    if (imu.begin_I2C(0x6B)) {
      imuAddress = 0x6B;
      return true;
    }
  }

  // Fallback in case scan missed it for some reason
  if (imu.begin_I2C(0x6A)) {
    imuAddress = 0x6A;
    return true;
  }

  if (imu.begin_I2C(0x6B)) {
    imuAddress = 0x6B;
    return true;
  }

  return false;
}

void setupIMUConfig() {
  imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
  imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
  imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("ESP32-S3 I2C + LSM6DSOX starting");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  scanI2C();

  if (!initIMU()) {
    Serial.println("Could not initialise LSM6DSOX.");
    Serial.println("Expected IMU at 0x6A or 0x6B.");
    Serial.println("If you see only 0x36, that is likely another onboard device, not the IMU.");
    imuReady = false;
    return;
  }

  setupIMUConfig();
  imuReady = true;

  Serial.print("LSM6DSOX initialised at 0x");
  if (imuAddress < 16) {
    Serial.print('0');
  }
  Serial.println(imuAddress, HEX);

  // CSV header for the Python script
  Serial.println("ax,ay,az,gx,gy,gz,temp");
}

void loop() {
  if (!imuReady) {
    delay(1000);
    return;
  }

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;

  imu.getEvent(&accel, &gyro, &temp);

  // Clean CSV only
  Serial.print(accel.acceleration.x, 4);
  Serial.print(",");
  Serial.print(accel.acceleration.y, 4);
  Serial.print(",");
  Serial.print(accel.acceleration.z, 4);
  Serial.print(",");
  Serial.print(gyro.gyro.x, 4);
  Serial.print(",");
  Serial.print(gyro.gyro.y, 4);
  Serial.print(",");
  Serial.print(gyro.gyro.z, 4);
  Serial.print(",");
  Serial.println(temp.temperature, 2);

  delay(50); // ~20 Hz
}