#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>

namespace WifiManager {

// ---------------------------------------------------------------------------
// Credentials
// ---------------------------------------------------------------------------
static constexpr const char* WIFI_SSID          = "bailey_phone";
static constexpr const char* WIFI_PASSWORD      = "abcdefgh";
static constexpr uint16_t    WEBSERVER_PORT     = 80;
static constexpr uint32_t    CONNECT_TIMEOUT_MS = 45000;

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------
struct ImuReading {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;
    float temp = 0.0f;
    bool  valid = false;
};

struct ImuSlot {
    char label[16] = {};
    ImuReading data;
};

enum class WifiState : uint8_t {
    CONNECTING,
    CONNECTED,
    FAILED
};

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static ImuSlot   imuSlots[2] = {};
static uint8_t   slotCount = 0;
static WebServer server(WEBSERVER_PORT);
static WifiState wifiState = WifiState::FAILED;
static uint32_t  connectStart = 0;
static bool      serverStarted = false;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void handleRoot();
static void handleData();

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
inline void safeAppend(char* buf, size_t bufSize, int& n, const char* fmt, ...) {
    if (n < 0 || static_cast<size_t>(n) >= bufSize) {
        n = static_cast<int>(bufSize - 1);
        buf[bufSize - 1] = '\0';
        return;
    }

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buf + n, bufSize - static_cast<size_t>(n), fmt, args);
    va_end(args);

    if (written < 0) {
        return;
    }

    n += written;
    if (static_cast<size_t>(n) >= bufSize) {
        n = static_cast<int>(bufSize - 1);
        buf[bufSize - 1] = '\0';
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
inline void wifiInit() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    connectStart = millis();
    wifiState = WifiState::CONNECTING;
    serverStarted = false;

    Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
}

inline void wifiHandle() {
    wl_status_t status = WiFi.status();

    if (wifiState == WifiState::CONNECTING) {
        if (status == WL_CONNECTED) {
            wifiState = WifiState::CONNECTED;

            if (!serverStarted) {
                server.on("/", handleRoot);
                server.on("/data", handleData);
                server.begin();
                serverStarted = true;
            }

            Serial.print("[WiFi] Connected, IP address: ");
            Serial.println(WiFi.localIP());
            Serial.printf("[WiFi] WebServer started on port %u\n", WEBSERVER_PORT);
        } else if (millis() - connectStart >= CONNECT_TIMEOUT_MS) {
            wifiState = WifiState::FAILED;
            Serial.println("[WiFi] Connection timed out - running without WiFi");
        }
        return;
    }

    if (wifiState == WifiState::FAILED) {
        return;
    }

    if (wifiState == WifiState::CONNECTED) {
        if (status != WL_CONNECTED) {
            wifiState = WifiState::FAILED;
            Serial.println("[WiFi] Lost connection");
            return;
        }

        server.handleClient();
    }
}

inline void wifiUpdateIMU(const char* label,
                          float ax, float ay, float az,
                          float gx, float gy, float gz,
                          float temp) {
    if (label == nullptr || label[0] == '\0') {
        return;
    }

    for (uint8_t i = 0; i < slotCount; i++) {
        if (strncmp(imuSlots[i].label, label, sizeof(imuSlots[i].label)) == 0) {
            imuSlots[i].data.ax = ax;
            imuSlots[i].data.ay = ay;
            imuSlots[i].data.az = az;
            imuSlots[i].data.gx = gx;
            imuSlots[i].data.gy = gy;
            imuSlots[i].data.gz = gz;
            imuSlots[i].data.temp = temp;
            imuSlots[i].data.valid = true;
            return;
        }
    }

    if (slotCount < 2) {
        ImuSlot& slot = imuSlots[slotCount];
        strncpy(slot.label, label, sizeof(slot.label) - 1);
        slot.label[sizeof(slot.label) - 1] = '\0';

        slot.data.ax = ax;
        slot.data.ay = ay;
        slot.data.az = az;
        slot.data.gx = gx;
        slot.data.gy = gy;
        slot.data.gz = gz;
        slot.data.temp = temp;
        slot.data.valid = true;

        slotCount++;
    }
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------
static void handleRoot() {
    static char buf[2400];
    int n = 0;
    buf[0] = '\0';

    safeAppend(buf, sizeof(buf), n,
               "<!DOCTYPE html><html><head>"
               "<meta charset='utf-8'>"
               "<meta http-equiv='refresh' content='1'>"
               "<title>CBST IMU Data</title>"
               "<style>"
               "body{font-family:monospace;}"
               "table{border-collapse:collapse;margin-bottom:16px;}"
               "td,th{border:1px solid #ccc;padding:4px 8px;}"
               "</style>"
               "</head><body>"
               "<h2>CBST Live IMU Data</h2>");

    bool anyValid = false;

    for (uint8_t i = 0; i < slotCount; i++) {
        const ImuSlot& s = imuSlots[i];
        if (!s.data.valid) {
            continue;
        }

        anyValid = true;

        safeAppend(buf, sizeof(buf), n,
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
        safeAppend(buf, sizeof(buf), n, "<p>Waiting for IMU data...</p>");
    }

    safeAppend(buf, sizeof(buf), n,
               "<p><a href=\"/data\">JSON endpoint</a></p>"
               "</body></html>");

    server.send(200, "text/html", buf);
}

static void handleData() {
    static char buf[700];
    int n = 0;
    buf[0] = '\0';

    safeAppend(buf, sizeof(buf), n, "{\"imu\":[");

    for (uint8_t i = 0; i < 2; i++) {
        if (i > 0) {
            safeAppend(buf, sizeof(buf), n, ",");
        }

        if (i < slotCount) {
            const ImuSlot& s = imuSlots[i];
            safeAppend(buf, sizeof(buf), n,
                       "{\"label\":\"%s\",\"ax\":%.4f,\"ay\":%.4f,\"az\":%.4f,"
                       "\"gx\":%.4f,\"gy\":%.4f,\"gz\":%.4f,\"temp\":%.2f,\"valid\":%s}",
                       s.label,
                       s.data.ax, s.data.ay, s.data.az,
                       s.data.gx, s.data.gy, s.data.gz,
                       s.data.temp,
                       s.data.valid ? "true" : "false");
        } else {
            safeAppend(buf, sizeof(buf), n,
                       "{\"label\":\"\",\"ax\":0.0000,\"ay\":0.0000,\"az\":0.0000,"
                       "\"gx\":0.0000,\"gy\":0.0000,\"gz\":0.0000,\"temp\":0.00,\"valid\":false}");
        }
    }

    safeAppend(buf, sizeof(buf), n, "]}");
    server.send(200, "application/json", buf);
}

}  // namespace WifiManager