#pragma once
// =============================================================
//  data_buffer.h
//  A simple circular buffer that stores IMU samples in memory.
//
//  Capacity: 2000 samples (~100 seconds at 20 Hz)
//  When full, oldest samples are overwritten (ring buffer).
//  On GET request, all samples are returned then buffer clears.
// =============================================================

#include "imu_manager.h"

static const size_t BUFFER_CAPACITY = 2000;

static ImuSample _buffer[BUFFER_CAPACITY];
static size_t    _head  = 0;   // next write position
static size_t    _count = 0;   // number of valid samples

// Add a sample to the buffer.
void bufferPush(const ImuSample& sample) {
    _buffer[_head] = sample;
    _head = (_head + 1) % BUFFER_CAPACITY;
    if (_count < BUFFER_CAPACITY) _count++;
}

// Number of samples currently stored.
size_t bufferCount() { return _count; }

// Write all stored samples as CSV rows to a WiFiClient, then clear.
// CSV header is sent first so the Python script can always rely on it.
void bufferFlushToClient(WiFiClient& client) {
    // Header — COLUMN ORDER: do not change without updating receiver.py
    client.println("ax,ay,az,gx,gy,gz,temp");

    // Oldest sample is at (_head - _count) wrapped around
    size_t start = (_head + BUFFER_CAPACITY - _count) % BUFFER_CAPACITY;
    for (size_t i = 0; i < _count; i++) {
        size_t idx = (start + i) % BUFFER_CAPACITY;
        client.println(imuToCSV(_buffer[idx]));
    }

    client.println("END");

    Serial.printf("[Buffer] Flushed %zu samples. Buffer cleared.\n", _count);

    // Reset
    _head  = 0;
    _count = 0;
}