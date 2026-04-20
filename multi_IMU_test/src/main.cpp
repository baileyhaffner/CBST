#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>

static const int SDA_PIN = 8;
static const int SCL_PIN = 9;

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
  if (found6A || true) {
    if (imu6A.begin_I2C(0x6A)) {
      setupSingleIMUConfig(imu6A);
      imu6AReady = true;
      Serial.println("LSM6DSOX initialised at 0x6A");
    } else {
      Serial.println("Could not initialise LSM6DSOX at 0x6A");
    }
  }

  // Try 0x6B
  if (found6B || true) {
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

void printIMUData(Adafruit_LSM6DSOX &imu, const char *label) {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;

  imu.getEvent(&accel, &gyro, &temp);

  // Labeled CSV row
  Serial.print(label);
  Serial.print(",");
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
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("ESP32-S3 dual LSM6DSOX I2C starting");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  scanI2C();

  if (!initIMUs()) {
    Serial.println("Could not initialise any LSM6DSOX sensors.");
    Serial.println("Expected IMUs at 0x6A and/or 0x6B.");
    Serial.println("If you only see 0x36, that is likely another onboard device, not the IMUs.");
    return;
  }

  // CSV header for Python
  Serial.println("imu,ax,ay,az,gx,gy,gz,temp");
}

void loop() {
  if (!imu6AReady && !imu6BReady) {
    delay(1000);
    return;
  }

  // Alternate between IMUs each loop pass, clearly labeled
  if (imu6AReady) {
    printIMUData(imu6A, "IMU_0x6A");
  }

  if (imu6BReady) {
    printIMUData(imu6B, "IMU_0x6B");
  }

  delay(50);
}