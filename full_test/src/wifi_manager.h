#pragma once

#include <Arduino.h>
#include <WiFi.h>

// ----------------------- EDIT THESE -----------------------
static const char* WIFI_SSID     = "bailey_phone";
static const char* WIFI_PASSWORD = "abcdefgh";
// ----------------------------------------------------------

static const uint16_t TCP_PORT            = 5005;
static const uint32_t CONNECT_TIMEOUT_MS  = 20000;

static WiFiServer tcpServer(TCP_PORT);
static WiFiClient tcpClient;

// ----------------------------------------------------------
// Scan networks multiple times to improve detection reliability
// ----------------------------------------------------------
void wifiScanNetworks() {
    const int SCAN_ATTEMPTS = 3;

    for (int attempt = 0; attempt < SCAN_ATTEMPTS; attempt++) {
        Serial.printf("\n[WiFi] Scan attempt %d/%d...\n", attempt + 1, SCAN_ATTEMPTS);

        // Longer scan: 500ms per channel (~6–7 seconds total)
        int n = WiFi.scanNetworks(false, true, false, 500);

        if (n <= 0) {
            Serial.println("[WiFi] No networks found.");
        } else {
            Serial.printf("[WiFi] Found %d network(s):\n", n);

            for (int i = 0; i < n; i++) {
                Serial.printf("  %2d: %s | RSSI %d | Ch %d | %s\n",
                              i + 1,
                              WiFi.SSID(i).c_str(),
                              WiFi.RSSI(i),
                              WiFi.channel(i),
                              (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "SECURED");
            }
        }

        delay(1000);  // short pause between scans
    }
}

// ----------------------------------------------------------
// Connect to WiFi
// ----------------------------------------------------------
bool wifiConnect() {
    WiFi.disconnect(true, true);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    // Run multiple scans before attempting connection
    wifiScanNetworks();

    Serial.printf("\n[WiFi] Connecting to SSID: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    wl_status_t lastStatus = WL_IDLE_STATUS;

    while (millis() - start < CONNECT_TIMEOUT_MS) {
        wl_status_t status = WiFi.status();

        if (status != lastStatus) {
            Serial.printf("[WiFi] Status changed: %d\n", status);
            lastStatus = status;
        }

        if (status == WL_CONNECTED) {
            Serial.println("[WiFi] Connected successfully.");
            Serial.printf("[WiFi] IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] TCP server listening on port %d\n", TCP_PORT);
            tcpServer.begin();
            return true;
        }

        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("[WiFi] FAILED to connect.");
    Serial.println("[WiFi] Likely causes:");
    Serial.println("  - hotspot not visible on 2.4 GHz");
    Serial.println("  - wrong SSID/password");
    Serial.println("  - unsupported hotspot security mode");

    return false;
}

// ----------------------------------------------------------
// Handle incoming client connections
// ----------------------------------------------------------
WiFiClient wifiGetClient() {
    if (tcpClient && tcpClient.connected()) {
        return tcpClient;
    }

    WiFiClient incoming = tcpServer.accept();
    if (incoming) {
        tcpClient = incoming;
        Serial.printf("[WiFi] Client connected from: %s\n",
                      tcpClient.remoteIP().toString().c_str());
    }

    return tcpClient;
}