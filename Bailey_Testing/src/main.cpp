#include <Arduino.h>
#include "imu_manager.h"
#include "wifi_manager.h"

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("ESP32-S3 dual LSM6DSOX I2C starting");

    ImuManager::beginBusAndButton();
    ImuManager::scanI2C();

    if (!ImuManager::initIMUs()) {
        Serial.println("Could not initialise any LSM6DSOX sensors.");
        Serial.println("Expected IMUs at 0x6A and/or 0x6B.");
        Serial.println("If you only see 0x36, that is likely another onboard device, not the IMUs.");
        return;
    }

    WifiManager::wifiInit();

    // CSV header
    Serial.println("imu,ax,ay,az,gx,gy,gz,temp");
}

void loop() {
    WifiManager::wifiHandle();

    ImuManager::serviceAndPublish(
        [](const char* label,
           float ax, float ay, float az,
           float gx, float gy, float gz,
           float temp) {
            WifiManager::wifiUpdateIMU(label, ax, ay, az, gx, gy, gz, temp);
        }
    );
}