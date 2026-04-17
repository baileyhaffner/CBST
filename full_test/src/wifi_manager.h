#pragma once

#include <Arduino.h>
#include <WiFi.h>

static const char* WIFI_SSID     = "bailey_phone";
static const char* WIFI_PASSWORD = "abcdefgh";

static const uint16_t TCP_PORT           = 5005;
static const uint32_t CONNECT_TIMEOUT_MS = 20000;

static WiFiServer tcpServer(TCP_PORT);
static WiFiClient tcpClient;

bool wifiConnect() {
    Serial.printf("\n[WiFi] Connecting to SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);

        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Serial.println("\n[WiFi] FAILED to connect.");
            Serial.printf("[WiFi] Final status: %d\n", WiFi.status());
            return false;
        }
    }

    Serial.println();
    Serial.println("[WiFi] Connected successfully.");
    Serial.printf("[WiFi] IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] TCP server listening on port %d\n", TCP_PORT);

    tcpServer.begin();
    return true;
}

WiFiClient wifiGetClient() {
    if (tcpClient && tcpClient.connected()) {
        return tcpClient;
    }

    WiFiClient incoming = tcpServer.accept();
    if (incoming) {
        tcpClient = incoming;
        Serial.printf("[WiFi] Client connected: %s\n",
                      tcpClient.remoteIP().toString().c_str());
    }

    return tcpClient;
}