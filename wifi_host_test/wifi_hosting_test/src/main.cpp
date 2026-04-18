/**
 * =============================================================
 claude and chatGPT wont help.
 Trying to use station mode
 it works sometimes but rarely
 * =============================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>   // Built into ESP32 Arduino core for ProS3

// ---------------------------------------------
//  SSID and Password
// -----------------------------------------------------------
const char* WIFI_SSID     = "Telstra68048F";
const char* WIFI_PASSWORD = "err2d583e6";
// --------------------------------------------------------


#define RGB_PIN        18
#define RGB_PWR_PIN    17          // Must be HIGH to power the LED
#define NUM_PIXELS     1

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// How often to print status while connected (ms)
const uint32_t STATUS_INTERVAL_MS = 5000;

// WiFi reconnect timeout
const uint32_t CONNECT_TIMEOUT_MS = 15000;

// ---------------------------------------------------------------
//  LED helpers
// ---------------------------------------------------------------
void setLED(uint8_t r, uint8_t g, uint8_t b) {
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}

void ledOff()     { setLED(0,   0,   0); }
void ledYellow()  { setLED(40,  30,  0); }   // dim to avoid blinding
void ledGreen()   { setLED(0,   40,  0); }
void ledRed()     { setLED(40,  0,   0); }

// ---------------------------------------------------------------
//  Connect to WiFi — blocks until connected or timeout
//  Returns true if successful
// ---------------------------------------------------------------

bool connectWiFi() {
    Serial.println();
    Serial.println("==============================================");
    Serial.println("  WiFi Connection Test — ProS3 ESP32-S3");
    Serial.println("==============================================");
    Serial.printf("  SSID     : %s\n", WIFI_SSID);
    Serial.printf("  MAC Addr : %s\n", WiFi.macAddress().c_str());
    Serial.println("----------------------------------------------");
    Serial.print("  Connecting");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();

    // Put in the ccdr? try next monday 
    uint8_t  dots  = 0;

    while (WiFi.status() != WL_CONNECTED) {
        ledYellow();
        delay(250);
        ledOff();
        delay(250);

        Serial.print(".");
        dots++;

        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Serial.println();
            Serial.println("  [FAIL] Timed out after 15 s.");
            Serial.println("  Check SSID / password and try again.");
            ledRed();
            return false;
        }
    }

    Serial.println();
    Serial.println("  [OK] Connected!");
    Serial.println("----------------------------------------------");
    Serial.printf("  IP Address : %s\n",  WiFi.localIP().toString().c_str());
    Serial.printf("  Subnet     : %s\n",  WiFi.subnetMask().toString().c_str());
    Serial.printf("  Gateway    : %s\n",  WiFi.gatewayIP().toString().c_str());
    Serial.printf("  DNS        : %s\n",  WiFi.dnsIP().toString().c_str());
    Serial.printf("  RSSI       : %d dBm\n", WiFi.RSSI());
    Serial.println("==============================================");

    ledGreen();
    return true;
}

// ---------------------------------------------------------------
//  Print a one-line status heartbeat
// ---------------------------------------------------------------
void printStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[STATUS] Connected | IP: %s | RSSI: %d dBm | Uptime: %lus\n",
            WiFi.localIP().toString().c_str(),
            WiFi.RSSI(),
            millis() / 1000);
        ledGreen();
    } else {
        Serial.println("[STATUS] !!! WiFi connection LOST — attempting reconnect...");
        ledRed();
        delay(1000);
        connectWiFi();   // auto-reconnect
    }
}

// ---------------------------------------------------------------
//  Setup
// ---------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);   // give Serial Monitor time to open

    // Power up the RGB LED
    pinMode(RGB_PWR_PIN, OUTPUT);
    digitalWrite(RGB_PWR_PIN, HIGH);
    pixel.begin();
    pixel.setBrightness(80);
    ledYellow();

    connectWiFi();
}

// ---------------------------------------------------------------
//  Loop — heartbeat every STATUS_INTERVAL_MS
// ---------------------------------------------------------------
void loop() {
    static uint32_t lastStatus = 0;

    if (millis() - lastStatus >= STATUS_INTERVAL_MS) {
        lastStatus = millis();
        printStatus();
    }
}