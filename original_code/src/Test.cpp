#include <Wire.h>
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    digitalWrite(7, HIGH);
    delay(2000);
    Wire.begin(8, 9);  // SDA=IO8, SCL=IO9 - adjust to your actual pins
    Serial.println("I2C Scanner starting...");
}

void loop() {
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Device found at 0x");
            Serial.println(addr, HEX);
        }
    }
    Serial.println("Scan complete.\n");
    delay(3000);
}