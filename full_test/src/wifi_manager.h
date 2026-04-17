#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

// ----------------------- WIFI -----------------------
static const char* WIFI_SSID     = "bailey_phone";
static const char* WIFI_PASSWORD = "abcdefgh";
// ----------------------------------------------------

static const uint16_t TCP_PORT           = 5005;
static const uint32_t CONNECT_TIMEOUT_MS = 20000;

// ----------------------- LED ------------------------
// ProS3 onboard RGB LED is on GPIO 48
static const int LED_PIN = 48;
static const int LED_COUNT = 1;

static Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ----------------------------------------------------
static WiFiServer tcpServer(TCP_PORT);
static WiFiClient tcpClient;

// Helper to set LED color
void setLED(uint8_t r, uint8_t g, uint8_t b) {
    led.setPixelColor(0, led.Color(r, g, b));
    led.show();
}

// ----------------------------------------------------
// Connect to WiFi with LED feedback
// ----------------------------------------------------
bool wifiConnect() {
    Serial.printf("\n[WiFi] Connecting to SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    delay(200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    wl_status_t lastStatus = WL_IDLE_STATUS;

    bool ledState = false;

    while (WiFi.status() != WL_CONNECTED) {
        wl_status_t status = WiFi.status();

        if (status != lastStatus) {
            Serial.printf("[WiFi] Status changed: %d\n", status);
            lastStatus = status;
        }

        // Flash blue while connecting
        ledState = !ledState;
        if (ledState) setLED(0, 0, 50);   // dim blue
        else          setLED(0, 0, 0);

        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Serial.println("\n[WiFi] FAILED to connect.");

            // Solid red on failure
            setLED(50, 0, 0);
            return false;
        }

        delay(500);
        Serial.print(".");
    }

    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] TCP server listening on port %d\n", TCP_PORT);

    // Solid green on success
    setLED(0, 50, 0);

    tcpServer.begin();
    return true;
}

// ----------------------------------------------------
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