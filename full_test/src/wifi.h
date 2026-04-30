#pragma once

#include <Arduino.h>
#include <WiFi.h>   
#include <vector>

const char* AP_SSID = "ProS3_Test_Network";
const char* AP_PASSWORD = "12345678";

const uint16_t SERVER_PORT = 3333;

WiFiServer wifiServer(SERVER_PORT);
WiFiClient wifiClient;

void wifiInit() {
    Serial.println();
    Serial.println("[WiFi] Starting ESP32 Wi-Fi Access Point...");

    WiFi.mode(WIFI_AP);
    delay(1000);

    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false);

    if (apStarted) {
        Serial.println("[WiFi] Access Point started successfully.");
        Serial.print("[WiFi] SSID: ");
        Serial.println(AP_SSID);
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[WiFi] ERROR: AP failed to start");
    }

    wifiServer.begin();
    Serial.println("[WiFi] TCP server started on port 3333");
}

void wifiHandle() {
    if (!wifiClient || !wifiClient.connected()) {
        WiFiClient newClient = wifiServer.available();

        if (newClient) {
            wifiClient = newClient;
            Serial.println("[WiFi] Client connected");
        }
    }
}

bool wifiClientConnected() {
    return wifiClient && wifiClient.connected();
}

void wifiSendCSVBuffer(const std::vector<String>& csvBuffer) {
    if (!wifiClientConnected()) {
        Serial.println("[WiFi] ERROR: No client connected");
        return;
    }

    Serial.println("[WiFi] Sending CSV...");

    wifiClient.println("START_CSV");
    wifiClient.println("sample,time_ms,imu,ax,ay,az,gx,gy,gz,temp");

    for (const String& line : csvBuffer) {
        wifiClient.println(line);
    }

    wifiClient.println("END_CSV");

    Serial.println("[WiFi] CSV sent");
}