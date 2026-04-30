#pragma once

#include <Arduino.h>
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
        Serial.print("[WiFi] Network name: ");
        Serial.println(AP_SSID);
        Serial.print("[WiFi] Password: ");
        Serial.println(AP_PASSWORD);
        Serial.print("[WiFi] ESP32 IP address: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[WiFi] ERROR: Failed to start Access Point.");
    }

    wifiServer.begin();

    Serial.print("[WiFi] TCP server started on port ");
    Serial.println(SERVER_PORT);
}

void wifiHandle() {
    if (!wifiClient || !wifiClient.connected()) {
        WiFiClient newClient = wifiServer.available();

        if (newClient) {
            wifiClient = newClient;
            Serial.println("[WiFi] Computer/client connected.");
        }
    }
}

bool wifiClientConnected() {
    return wifiClient && wifiClient.connected();
}

void wifiSendCSVBuffer(const std::vector<String>& csvBuffer) {
    wifiHandle();

    if (!wifiClientConnected()) {
        Serial.println("[WiFi] ERROR: No TCP client connected. CSV buffer was not sent.");
        Serial.println("[WiFi] Connect computer to Wi-Fi, then connect TCP client to 192.168.4.1:3333.");
        return;
    }

    Serial.println("[WiFi] Sending CSV buffer...");

    wifiClient.println("START_CSV");
    wifiClient.println("sample,time_ms,imu,ax,ay,az,gx,gy,gz,temp");

    for (const String& line : csvBuffer) {
        wifiClient.println(line);
        delay(1);
    }

    wifiClient.println("END_CSV");

    Serial.print("[WiFi] CSV transmission complete. Lines sent: ");
    Serial.println(csvBuffer.size());
}