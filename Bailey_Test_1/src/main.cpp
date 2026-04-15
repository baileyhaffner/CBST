#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

// Change these if your wiring is different
static const int SDA_PIN = 8;
static const int SCL_PIN = 9;

Adafruit_LSM6DSOX imu;

bool scanI2C(uint8_t &foundAddress) {
  bool foundAny = false;
  foundAddress = 0;

  Serial.println();
  Serial.println("Scanning I2C bus...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found I2C device at 0x");
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);

      if (!foundAny) {
        foundAddress = addr;
        foundAny = true;
      }
    } else if (error == 4) {
      Serial.print("Unknown error at 0x");
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);
    }
  }

  if (!foundAny) {
    Serial.println("No I2C devices found.");
  }

  return foundAny;
}

void printIMUData() {
  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);

  Serial.println("---- IMU Data ----");

  Serial.print("Accel X: ");
  Serial.print(accel.acceleration.x, 3);
  Serial.print("  Y: ");
  Serial.print(accel.acceleration.y, 3);
  Serial.print("  Z: ");
  Serial.println(accel.acceleration.z, 3);

  Serial.print("Gyro  X: ");
  Serial.print(gyro.gyro.x, 3);
  Serial.print("  Y: ");
  Serial.print(gyro.gyro.y, 3);
  Serial.print("  Z: ");
  Serial.println(gyro.gyro.z, 3);

  Serial.print("Temp C: ");
  Serial.println(temp.temperature, 2);

  Serial.println("------------------");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("ESP32-S3 I2C + LSM6DSOX test starting");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  uint8_t firstFound = 0;
  bool found = scanI2C(firstFound);

  if (!found) {
    Serial.println("No devices found. Check SDA, SCL, power, and GND.");
    return;
  }

  Serial.println();
  Serial.println("Trying to initialise LSM6DSOX...");

  // Try default address first
  if (imu.begin_I2C()) {
    Serial.println("LSM6DSOX initialised at default address.");
  } else if (firstFound != 0 && imu.begin_I2C(firstFound)) {
    Serial.print("LSM6DSOX initialised at scanned address 0x");
    if (firstFound < 16) Serial.print('0');
    Serial.println(firstFound, HEX);
  } else {
    Serial.println("Found I2C device(s), but could not initialise LSM6DSOX.");
    Serial.println("If the found address is not 0x6A or 0x6B, it may be another device (dev board BMS at 0x36)");
    return;
  }

  imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  imu.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
  imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
  imu.setGyroDataRate(LSM6DS_RATE_104_HZ);

  Serial.println("IMU setup complete.");
}

void loop() {
  static bool imuReadyChecked = false;
  static bool imuReady = false;

  if (!imuReadyChecked) {
    sensors_event_t accel, gyro, temp;
    if (imu.getEvent(&accel, &gyro, &temp), true) {
      imuReady = true;
    }
    imuReadyChecked = true;
  }

  if (imuReady) {
    printIMUData();
  } else {
    Serial.println("IMU not ready.");
  }

  delay(500);
}