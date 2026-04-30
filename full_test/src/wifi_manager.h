#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

// Wi-Fi config
const char* AP_SSID = "ProS3_Test_Network";
const char* AP_PASSWORD = "12345678";

// TCP server
const uint16_t SERVER_PORT = 3333;
WiFiServer server(SERVER_PORT);

// Global client
WiFiClient client;

// Timing for test/debug
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000;

// =======================
// INIT
// =======================
void wifiInit() {
    Serial.println();
    Serial.println("[WiFi] Starting ESP32 Access Point...");

    WiFi.mode(WIFI_AP);

    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

    if (apStarted) {
        Serial.println("[WiFi] Access Point started.");
        Serial.print("[WiFi] SSID: ");
        Serial.println(AP_SSID);
        Serial.print("[WiFi] Password: ");
        Serial.println(AP_PASSWORD);
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[WiFi] ERROR: Failed to start AP");
    }

    server.begin();

    Serial.print("[WiFi] TCP server started on port ");
    Serial.println(SERVER_PORT);
}

// =======================
// HANDLE
// =======================
void wifiHandle() {
    if (!client || !client.connected()) {
        client = server.available();

        if (client) {
            Serial.println("[WiFi] Client connected");
        }
    }

    if (client && client.connected()) {
        unsigned long currentTime = millis();

        if (currentTime - lastSendTime >= sendInterval) {
            lastSendTime = currentTime;

            client.println("test working");
            Serial.println("[WiFi] Sent: test working");
        }
    }
}

// =======================
// SEND CSV BUFFER
// =======================
void wifiSendCSVBuffer(const std::vector<String>& csvBuffer) {
    if (!client || !client.connected()) {
        Serial.println("[WiFi] ERROR: No client connected");
        return;
    }

    Serial.println("[WiFi] Sending CSV buffer...");

    client.println("START_CSV");
    client.println("sample,time_ms,imu,ax,ay,az,gx,gy,gz,temp");

    for (const String& line : csvBuffer) {
        client.println(line);
        delay(1);
    }

    client.println("END_CSV");

    Serial.println("[WiFi] CSV sent successfully");
}