#pragma once

// NOTE: Include this header from exactly one translation unit (main.cpp only).
// The static module-level variables below each get a separate copy per TU.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Credentials — fill in SSID and PASSWORD manually before flashing
// ---------------------------------------------------------------------------
static constexpr const char* WIFI_SSID          = "bailey_phone";
static constexpr const char* WIFI_PASSWORD       = "abcdefgh";
static constexpr uint16_t    WEBSERVER_PORT      = 80;
static constexpr uint32_t    CONNECT_TIMEOUT_MS  = 45000;

// ---------------------------------------------------------------------------
// Data structures (static allocation, no heap)
// ---------------------------------------------------------------------------
struct ImuReading {
    float ax, ay, az, gx, gy, gz, temp;
    bool  valid;
};

struct ImuSlot {
    char       label[16];
    ImuReading data;
};

enum class WifiState : uint8_t { CONNECTING, CONNECTED, FAILED };

static ImuSlot   _imuSlots[2]   = {};
static uint8_t   _slotCount     = 0;
static WebServer _server(WEBSERVER_PORT);
static WifiState _wifiState     = WifiState::CONNECTING;
static uint32_t  _connectStart  = 0;

// ---------------------------------------------------------------------------
// HTTP handlers (forward declarations)
// ---------------------------------------------------------------------------
static void handleRoot();
static void handleData();

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Call once in setup() — starts connection attempt, returns immediately (non-blocking)
inline void wifiInit() {
    WiFi.useStaticBuffers(true);  // MUST be first
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
    _connectStart = millis();
    _wifiState    = WifiState::CONNECTING;
}

// Call first in every loop() iteration — polls connection state, serves HTTP
inline void wifiHandle() {
    if (_wifiState == WifiState::CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _wifiState = WifiState::CONNECTED;
            Serial.print("[WiFi] Connected, IP address: ");
            Serial.println(WiFi.localIP());
            _server.on("/",     handleRoot);
            _server.on("/data", handleData);
            _server.begin();
            Serial.printf("[WiFi] WebServer started on port %d\n", WEBSERVER_PORT);
        } else if (millis() - _connectStart >= CONNECT_TIMEOUT_MS) {
            _wifiState = WifiState::FAILED;
            Serial.printf("[WiFi] Connection timed out - running without WiFi\n");
        }
        return;
    }

    if (_wifiState == WifiState::CONNECTED) {
        _server.handleClient();
    }
}

// Update a named IMU slot — call after every getEvent() read
inline void wifiUpdateIMU(const char* label,
                          float ax, float ay, float az,
                          float gx, float gy, float gz,
                          float temp) {
    for (uint8_t i = 0; i < _slotCount; i++) {
        if (strcmp(_imuSlots[i].label, label) == 0) {
            _imuSlots[i].data = { ax, ay, az, gx, gy, gz, temp, true };
            return;
        }
    }
    if (_slotCount < 2) {
        ImuSlot& slot = _imuSlots[_slotCount];
        strncpy(slot.label, label, 15);
        slot.label[15] = '\0';
        slot.data      = { ax, ay, az, gx, gy, gz, temp, true };
        _slotCount++;
    }
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------
static void handleRoot() {
    static char buf[2400];
    int n = 0;

    n += snprintf(buf + n, sizeof(buf) - n,
        "<!DOCTYPE html><html><head>"
        "<meta charset='utf-8'>"
        "<meta http-equiv='refresh' content='1'>"
        "<title>CBST IMU Data</title>"
        "<style>body{font-family:monospace;} table{border-collapse:collapse;} "
        "td,th{border:1px solid #ccc;padding:4px 8px;}</style>"
        "</head><body>"
        "<h2>CBST Live IMU Data</h2>");

    bool anyValid = false;
    for (uint8_t i = 0; i < _slotCount; i++) {
        const ImuSlot& s = _imuSlots[i];
        if (!s.data.valid) continue;
        anyValid = true;

        n += snprintf(buf + n, sizeof(buf) - n,
            "<h3>%s</h3>"
            "<table>"
            "<tr><th>ax</th><td>%.4f</td></tr>"
            "<tr><th>ay</th><td>%.4f</td></tr>"
            "<tr><th>az</th><td>%.4f</td></tr>"
            "<tr><th>gx</th><td>%.4f</td></tr>"
            "<tr><th>gy</th><td>%.4f</td></tr>"
            "<tr><th>gz</th><td>%.4f</td></tr>"
            "<tr><th>temp</th><td>%.2f</td></tr>"
            "</table>",
            s.label,
            s.data.ax, s.data.ay, s.data.az,
            s.data.gx, s.data.gy, s.data.gz,
            s.data.temp);
    }

    if (!anyValid) {
        n += snprintf(buf + n, sizeof(buf) - n, "<p>Waiting for IMU data...</p>");
    }

    n += snprintf(buf + n, sizeof(buf) - n,
        "<p><a href=\"/data\">JSON endpoint</a></p>"
        "</body></html>");

    _server.send(200, "text/html", buf);
}

static void handleData() {
    static char buf[700];
    int n = 0;

    n += snprintf(buf + n, sizeof(buf) - n, "{\"imu\":[");

    for (uint8_t i = 0; i < 2; i++) {
        if (i > 0) n += snprintf(buf + n, sizeof(buf) - n, ",");

        if (i < _slotCount) {
            const ImuSlot& s = _imuSlots[i];
            n += snprintf(buf + n, sizeof(buf) - n,
                "{\"label\":\"%s\",\"ax\":%.4f,\"ay\":%.4f,\"az\":%.4f,"
                "\"gx\":%.4f,\"gy\":%.4f,\"gz\":%.4f,\"temp\":%.2f,\"valid\":%s}",
                s.label,
                s.data.ax, s.data.ay, s.data.az,
                s.data.gx, s.data.gy, s.data.gz,
                s.data.temp,
                s.data.valid ? "true" : "false");
        } else {
            n += snprintf(buf + n, sizeof(buf) - n,
                "{\"label\":\"\",\"ax\":0.0000,\"ay\":0.0000,\"az\":0.0000,"
                "\"gx\":0.0000,\"gy\":0.0000,\"gz\":0.0000,\"temp\":0.00,\"valid\":false}");
        }
    }

    n += snprintf(buf + n, sizeof(buf) - n, "]}");
    _server.send(200, "application/json", buf);
}
