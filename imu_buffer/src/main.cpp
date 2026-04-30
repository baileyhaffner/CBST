#include <Arduino.h>
#include <Wire.h>
#include "imu_manager.h"
#include "buffer_manager.h"

static constexpr int SDA_PIN    = 8;
static constexpr int SCL_PIN    = 9;
static constexpr int BUTTON_PIN = 41;

static constexpr uint32_t IMU_READ_INTERVAL_MS = 50;

IMUManager imu;
BufferManager buffer;

bool lastButtonState = LOW;
bool loggingActive = false;

uint32_t lastRead = 0;
uint32_t sampleNumber = 0;

void handleSerialCommands() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "READ") {
        buffer.sendAll();
    }
    else if (cmd == "CLEAR") {
        buffer.clear();
        Serial.println("CLEARED");
    }
    else if (cmd == "STATUS") {
        buffer.printStatus();
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);

    pinMode(BUTTON_PIN, INPUT);

    buffer.begin();

    Serial.println();
    Serial.println("ESP32-S3 IMU Logger Starting");

    if (!imu.begin()) {
        Serial.println("No LSM6DSOX IMUs found.");
        Serial.println("Expected addresses: 0x6A and/or 0x6B");
    }

    buffer.printStatus();

    Serial.println("READY");
    Serial.println("Hold button to record. New button press clears old log.");
}

void loop() {
    handleSerialCommands();

    int buttonState = digitalRead(BUTTON_PIN);

    // Rising edge: button has just been pressed
    if (buttonState == HIGH && lastButtonState == LOW) {
        Serial.println("NEW SESSION - clearing old log");

        buffer.clear();
        buffer.startSession();

        loggingActive = true;
        sampleNumber = 0;
        lastRead = 0;
    }

    // Falling edge: button has just been released
    if (buttonState == LOW && lastButtonState == HIGH) {
        Serial.println("SESSION END");

        loggingActive = false;
        buffer.endSession();
    }

    lastButtonState = buttonState;

    if (!loggingActive) return;

    if (millis() - lastRead >= IMU_READ_INTERVAL_MS) {
        lastRead = millis();

        uint32_t timestamp = millis();

        IMUData dataA;
        IMUData dataB;

        if (imu.readIMU(0x6A, dataA, timestamp, sampleNumber)) {
            buffer.store(dataA);
        }

        if (imu.readIMU(0x6B, dataB, timestamp, sampleNumber)) {
            buffer.store(dataB);
        }

        sampleNumber++;
    }
}