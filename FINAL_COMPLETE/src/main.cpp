// ================================
// Libraries
// ================================
#include <Arduino.h>
#include <Wire.h>
#include "imu_manager.h"
#include "buffer_manager.h"

// ================================
// Pin configuration
// ================================
static constexpr int SDA_PIN    = 8;
static constexpr int SCL_PIN    = 9;
static constexpr int BUTTON_PIN = 41;

static constexpr uint32_t IMU_LOGGING_RATE_HZ  = 400;  // the SPIFFS write speed is struggling past 400Hz and getting a lot of drop.
static constexpr uint32_t IMU_SAMPLING_RATE_HZ = 833;  // from the data sheet this is the max data rate for this IMU

// every sample is doing an I2C read, formatting floats into CSV, and writting to SPIFFS.
// flush() is only called at the end of the session after the button is released. (different to our last code)
// I think the best way to pull more speed would be to buffer a bunch of samples in RAM and then write them all at once, but this is good enough for now and much simpler.
// Not sure how it works but FIFO with buffering on the IMU could be a lot better to max out the I2C bus and reduce the time spent doing I2C reads. 

// ================================
// Managers
// ================================
IMUManager imu;
BufferManager buffer;

// ================================
// Logging state
// ================================
bool lastButtonState = LOW;
bool loggingActive = false;

uint32_t lastRead = 0;
uint32_t sampleNumber = 0;

// ================================
// Serial command handler
// ================================
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

// ================================
// Setup
// ================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    pinMode(BUTTON_PIN, INPUT);

    buffer.begin();

    Serial.println();
    Serial.println("ESP32-S3 IMU Logger Starting");

    if (!imu.begin(IMU_SAMPLING_RATE_HZ)) {
        Serial.println("No LSM6DSOX IMUs found.");
        Serial.println("Expected addresses: 0x6A and/or 0x6B");
    }

    buffer.printStatus();

    Serial.println("READY");
    Serial.println("Hold button to record. New button press clears old log.");
}

// ================================
// Main loop
// ================================
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

    if (micros() - lastRead >= (1000000UL / IMU_LOGGING_RATE_HZ)) {
        lastRead = micros();

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