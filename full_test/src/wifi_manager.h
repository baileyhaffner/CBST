#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

const char* AP_SSID = "ProS3_Test_Network";
const char* AP_PASSWORD = "";   // open network for testing

const uint16_t SERVER_PORT = 3333;

WiFiServer server(SERVER_PORT);
WiFiClient client;

void wifiInit() {
    Serial.println();
    Serial.println("[WiFi] Starting ESP32 Access Point...");

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    delay(1000);

    bool apStarted = WiFi.softAP(
        AP_SSID,
        AP_PASSWORD,
        6,        // try 1, 6, or 11
        false,    // not hidden
        4
    );

    if (apStarted) {
        Serial.println("[WiFi] Access Point started.");
        Serial.print("[WiFi] SSID: ");
        Serial.println(AP_SSID);
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[WiFi] ERROR: Failed to start AP");
    }

    server.begin();
    Serial.print("[WiFi] TCP server started on port ");
    Serial.println(SERVER_PORT);
}

void wifiHandle() {
    static uint32_t lastStatusPrint = 0;

    if (!client || !client.connected()) {
        client = server.available();

        if (client) {
            Serial.println("[WiFi] TCP client connected");
        }
    }

    if (millis() - lastStatusPrint >= 5000) {
        lastStatusPrint = millis();

        Serial.print("[WiFi] Connected Wi-Fi stations: ");
        Serial.println(WiFi.softAPgetStationNum());

        Serial.print("[WiFi] AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
}

bool wifiClientConnected() {
    return client && client.connected();
}

void wifiSendCSVBuffer(const std::vector<String>& csvBuffer) {
    if (!wifiClientConnected()) {
        Serial.println("[WiFi] ERROR: No TCP client connected");
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

    Serial.print("[WiFi] CSV sent. Lines: ");
    Serial.println(csvBuffer.size());
}