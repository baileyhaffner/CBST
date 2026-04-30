#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

<<<<<<< HEAD
const char* AP_SSID = "ProS3_Test_Network";
const char* AP_PASSWORD = "";   // open network for testing

const uint16_t SERVER_PORT = 3333;

WiFiServer server(SERVER_PORT);
WiFiClient client;

=======
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
>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4
void wifiInit() {
    Serial.println();
    Serial.println("[WiFi] Starting ESP32 Access Point...");

    WiFi.mode(WIFI_AP);
<<<<<<< HEAD
    WiFi.setSleep(false);
    delay(1000);

    bool apStarted = WiFi.softAP(
        AP_SSID,
        AP_PASSWORD,
        6,        // try 1, 6, or 11
        false,    // not hidden
        4
    );
=======

    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4

    if (apStarted) {
        Serial.println("[WiFi] Access Point started.");
        Serial.print("[WiFi] SSID: ");
        Serial.println(AP_SSID);
<<<<<<< HEAD
=======
        Serial.print("[WiFi] Password: ");
        Serial.println(AP_PASSWORD);
>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[WiFi] ERROR: Failed to start AP");
    }

    server.begin();
<<<<<<< HEAD
=======

>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4
    Serial.print("[WiFi] TCP server started on port ");
    Serial.println(SERVER_PORT);
}

<<<<<<< HEAD
void wifiHandle() {
    static uint32_t lastStatusPrint = 0;

=======
// =======================
// HANDLE
// =======================
void wifiHandle() {
>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4
    if (!client || !client.connected()) {
        client = server.available();

        if (client) {
<<<<<<< HEAD
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
=======
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
>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4
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

<<<<<<< HEAD
    Serial.print("[WiFi] CSV sent. Lines: ");
    Serial.println(csvBuffer.size());
=======
    Serial.println("[WiFi] CSV sent successfully");
>>>>>>> 5628a4b03ed815a69c0c4ed2be07c8703c2af1b4
}