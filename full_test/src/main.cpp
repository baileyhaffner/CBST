// =============================================================
//  main.cpp
//  ESP32-S3 — IMU data logger with WiFi TCP server
//
//  Flow:
//    1. Connect to WiFi and start TCP server on port 5005
//    2. Continuously read IMU at ~20 Hz into a ring buffer
//    3. When Python sends "GET\n" — flush buffer as CSV + "END\n"
//    4. When Python sends "PING\n" — reply "PONG\n" (health check)
// =============================================================

#include <Arduino.h>
#include "wifi_manager.h"
#include "imu_manager.h"
#include "data_buffer.h"

static const uint32_t IMU_SAMPLE_INTERVAL_MS = 50;  // 50ms = ~20 Hz

bool imuReady  = false;
bool wifiReady = false;

// --------------- Handle incoming TCP command ------------------
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

// ----------------------- Setup --------------------------------
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== ESP32-S3 IMU Logger ===");

    wifiReady = wifiConnect();
    imuReady  = imuInit();

    if (imuReady) {
        Serial.println("[Main] IMU ready — buffering started.");
    } else {
        Serial.println("[Main] IMU not found — check wiring and I2C pins.");
    }

    if (wifiReady) {
        Serial.printf("[Main] Send GET to %s:%d to retrieve CSV data.\n",
                      WiFi.localIP().toString().c_str(), TCP_PORT);
    }
}

// ----------------------- Loop ---------------------------------
void loop() {
    static uint32_t lastSample = 0;

    // Sample IMU on interval
    if (imuReady && (millis() - lastSample >= IMU_SAMPLE_INTERVAL_MS)) {
        lastSample = millis();
        bufferPush(imuRead());
    }

    // Handle any incoming TCP commands
    if (wifiReady) {
        WiFiClient client = wifiGetClient();
        handleClient(client);
    }
}