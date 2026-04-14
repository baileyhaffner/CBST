#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>

Adafruit_LSM6DSOX imu;

void setup() {
  Serial.begin(115200);
  Wire.begin(13, 12); // or specify pins: Wire.begin(SDA, SCL);

  if (!imu.begin_I2C()) {
    Serial.println("Failed to find IMU");
    while (1);
  }
}

void loop() {
  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);

  Serial.print("Accel X: ");
  Serial.println(accel.acceleration.x);

  delay(500);
}