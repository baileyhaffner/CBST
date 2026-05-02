#pragma once

// ================================
// Libraries
// ================================
#include <Arduino.h>
#include <SPIFFS.h>
#include "imu_manager.h"

// ================================
// Buffer manager class
// ================================
class BufferManager {
public:
    static constexpr const char *LOG_PATH = "/imu_log.csv";

    File logFile;

    // ================================
    // Start SPIFFS storage
    // ================================
    bool begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS mount failed");
            return false;
        }

        Serial.println("SPIFFS mounted");
        return true;
    }

    // ================================
    // Clear saved log file
    // ================================
    void clear() {
        closeLogFile();

        if (SPIFFS.exists(LOG_PATH)) {
            SPIFFS.remove(LOG_PATH);
        }
    }

    // ================================
    // Start a new logging session
    // ================================
    void startSession() {
        closeLogFile();

        logFile = SPIFFS.open(LOG_PATH, FILE_WRITE);

        if (!logFile) {
            Serial.println("Failed to open log file for writing");
            return;
        }

        logFile.println("timestamp_ms,sample_number,imu,ax,ay,az,gx,gy,gz,temp");
        logFile.flush();
    }

    // ================================
    // End current logging session
    // ================================
    void endSession() {
        closeLogFile();
    }

    // ================================
    // Store one IMU sample
    // ================================
    void store(const IMUData &d) {
        if (!logFile) {
            return;
        }

        logFile.printf(
            "%lu,%lu,0x%02X,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
            static_cast<unsigned long>(d.timestamp_ms),
            static_cast<unsigned long>(d.sample_number),
            d.imu_id,
            d.ax,
            d.ay,
            d.az,
            d.gx,
            d.gy,
            d.gz,
            d.temp
        );

        logFile.flush();
    }

    // ================================
    // Send all stored data over Serial
    // ================================
    void sendAll() {
        closeLogFile();

        Serial.println("BEGIN_LOG");

        File f = SPIFFS.open(LOG_PATH, FILE_READ);

        if (!f) {
            Serial.println("NO_LOG_FILE");
            Serial.println("END_LOG");
            return;
        }

        while (f.available()) {
            Serial.write(f.read());
        }

        f.close();

        Serial.println();
        Serial.println("END_LOG");
    }

    // ================================
    // Print storage status
    // ================================
    void printStatus() {
        closeLogFile();

        Serial.println();
        Serial.println("Storage status:");

        if (!SPIFFS.exists(LOG_PATH)) {
            Serial.println("No log file stored");
            return;
        }

        File f = SPIFFS.open(LOG_PATH, FILE_READ);

        if (!f) {
            Serial.println("Log file exists but could not be opened");
            return;
        }

        Serial.print("Log file size: ");
        Serial.print(f.size());
        Serial.println(" bytes");

        f.close();
    }

private:
    // ================================
    // Safely close log file
    // ================================
    void closeLogFile() {
        if (logFile) {
            logFile.flush();
            logFile.close();
        }
    }
};