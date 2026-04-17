#include <Arduino.h>
#include <WiFi.h>
#include "wifi_manager.h"
#include "imu_manager.h"
#include "data_buffer.h"

static const uint32_t IMU_SAMPLE_INTERVAL_MS = 50;

bool imuReady  = false;
bool wifiReady = false;

void handleClient(WiFiClient& client) {
    if (!client || !client.connected()) return;

    if (client.available()) {
        String cmd = client.readStringUntil('\n');
        cmd.trim();

        if (cmd == "GET") {
            bufferFlushToClient(client);
        } else if (cmd == "PING") {
            client.println("PONG");
        } else {
            Serial.printf("[TCP] Unknown command: %s\n", cmd.c_str());
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESP32-S3 IMU Logger starting...");
    Serial.println("=================================");

    wifiReady = wifiConnect();
    imuReady  = imuInit();

    if (imuReady) {
        Serial.println("[Main] IMU ready - buffering started.");
    } else {
        Serial.println("[Main] IMU not found - check wiring and I2C pins.");
    }

    if (wifiReady) {
        Serial.printf("[Main] Board IP address: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[Main] WiFi not connected.");
    }
}

void loop() {
    static uint32_t lastSample = 0;

    if (imuReady && (millis() - lastSample >= IMU_SAMPLE_INTERVAL_MS)) {
        lastSample = millis();
        bufferPush(imuRead());
    }

    if (wifiReady) {
        WiFiClient client = wifiGetClient();
        handleClient(client);
    }
}