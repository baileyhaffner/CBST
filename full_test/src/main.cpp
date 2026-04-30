#include <Arduino.h>
#include <WiFi.h>
#include <vector>

#include "imu.h"
#include "wifi_manager.h"

std::vector<String> csvBuffer;

bool loggingActive = false;
bool previousButtonState = LOW;

uint32_t lastIMURead = 0;
uint32_t sampleNumber = 0;

const size_t MAX_BUFFER_LINES = 3000;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("ESP32-S3 ProS3[D] multi-IMU Wi-Fi logger starting...");

    pinMode(BUTTON_PIN, INPUT);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);

    scanI2C();

    if (!initIMUs()) {
        Serial.println("[Main] ERROR: Could not initialise any LSM6DSOX sensors.");
        Serial.println("[Main] Expected IMUs at 0x6A and/or 0x6B.");
    }

    wifiInit();

    Serial.println();
    Serial.println("[Main] Ready.");
    Serial.println("[Main] Hold button to log IMU data.");
    Serial.println("[Main] Release button to transmit CSV buffer over Wi-Fi.");
}

void loop() {
    wifiHandle();

    if (!imu6AReady && !imu6BReady) {
        static uint32_t lastErrorLog = 0;

        if (millis() - lastErrorLog >= IMU_ERROR_INTERVAL_MS) {
            lastErrorLog = millis();
            Serial.println("[Main] ERROR: No IMUs ready.");
        }

        return;
    }

    bool currentButtonState = digitalRead(BUTTON_PIN);

    if (currentButtonState == HIGH && previousButtonState == LOW) {
        csvBuffer.clear();
        sampleNumber = 0;
        loggingActive = true;

        Serial.println();
        Serial.println("[Main] Button pressed.");
        Serial.println("[Main] Old buffer cleared.");
        Serial.println("[Main] Logging started.");
    }

    if (currentButtonState == LOW && previousButtonState == HIGH) {
        loggingActive = false;

        Serial.println();
        Serial.println("[Main] Button released.");
        Serial.print("[Main] Logging stopped. Lines in buffer: ");
        Serial.println(csvBuffer.size());

        wifiSendCSVBuffer(csvBuffer);
    }

    if (loggingActive && millis() - lastIMURead >= IMU_READ_INTERVAL_MS) {
        lastIMURead = millis();

        if (csvBuffer.size() >= MAX_BUFFER_LINES) {
            Serial.println("[Main] WARNING: Buffer full. Stopping logging.");
            loggingActive = false;
        } else {
            sampleNumber++;

            if (imu6AReady) {
                String lineA = readIMUToCSVLine("IMU_0x6A", imu6A, sampleNumber);
                csvBuffer.push_back(lineA);
                Serial.println(lineA);
            }

            if (imu6BReady && csvBuffer.size() < MAX_BUFFER_LINES) {
                String lineB = readIMUToCSVLine("IMU_0x6B", imu6B, sampleNumber);
                csvBuffer.push_back(lineB);
                Serial.println(lineB);
            }
        }
    }

    previousButtonState = currentButtonState;
}