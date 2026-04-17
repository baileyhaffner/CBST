#pragma once
// =============================================================
//  wifi_manager.h
//  Handles WiFi connection and the TCP server socket.
//
//  TCP Protocol (dead simple):
//    Python sends : "GET\n"
//    ESP replies  : CSV rows, then "END\n" to signal done
//    Python sends : "PING\n"  →  ESP replies "PONG\n" (health check)
// =============================================================

#include <Arduino.h>
#include <WiFi.h>

// ---------------------------------------------------------------
//  !! EDIT THESE BEFORE FLASHING !!
// ---------------------------------------------------------------
static const char* WIFI_SSID     = "bailey_phone";
static const char* WIFI_PASSWORD = "abcdefgh";
// ---------------------------------------------------------------

static const uint16_t TCP_PORT          = 5005;
static const uint32_t CONNECT_TIMEOUT_MS = 15000;

static WiFiServer  tcpServer(TCP_PORT);
static WiFiClient  tcpClient;

// Connect to WiFi — blocks until connected or timeout.
// Returns true on success.
bool wifiConnect() {
    Serial.printf("\n[WiFi] Connecting to: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Serial.println("[WiFi] FAILED — check SSID/password.");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] TCP server listening on port %d\n", TCP_PORT);
    tcpServer.begin();
    return true;
}

// Call every loop iteration.
// Returns a connected WiFiClient if one is available, else invalid client.
WiFiClient wifiGetClient() {
    // Keep existing client if still connected
    if (tcpClient && tcpClient.connected()) {
        return tcpClient;
    }
    // Accept a new client if one is waiting
    WiFiClient incoming = tcpServer.accept();
    if (incoming) {
        tcpClient = incoming;
        Serial.printf("[WiFi] Client connected: %s\n",
                      tcpClient.remoteIP().toString().c_str());
    }
    return tcpClient;
}