#include <Arduino.h>
#include "wifi_helper.h"

const char* WIFI_SSID = "bailey_phone";
const char* WIFI_PASSWORD = "abcdefgh";

// Replace this with your computer's IP on the same Wi-Fi/hotspot network
const char* TARGET_IP = "172.20.10.3";
const uint16_t TARGET_PORT = 4210;

WifiHelper wifi(WIFI_SSID, WIFI_PASSWORD, TARGET_IP, TARGET_PORT);

unsigned long lastSend = 0;
unsigned long counter = 0;

void setup() {
    Serial.begin(115200);
    delay(10);

    wifi.begin();
}

void loop() {
    unsigned long now = millis();

    if (now - lastSend >= 1000) {
        lastSend = now;

        String msg = "Hello from ESP32 packet #" + String(counter++);
        wifi.sendTestPacket(msg);
    }
}