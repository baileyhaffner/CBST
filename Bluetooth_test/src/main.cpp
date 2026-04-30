#include <Arduino.h>
#include <WiFi.h>

// Wi-Fi network created by the ESP32
const char* AP_SSID = "ProS3_Test_Network";
const char* AP_PASSWORD = "12345678";   // Minimum 8 characters

// TCP server settings
const uint16_t SERVER_PORT = 3333;
WiFiServer server(SERVER_PORT);

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // 1 second

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting ESP32 Wi-Fi Access Point...");

  WiFi.mode(WIFI_AP);

  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (apStarted) {
    Serial.println("Access Point started successfully.");
    Serial.print("Wi-Fi name: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start Access Point.");
  }

  server.begin();
  Serial.print("TCP server started on port ");
  Serial.println(SERVER_PORT);
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Client connected.");

    while (client.connected()) {
      unsigned long currentTime = millis();

      if (currentTime - lastSendTime >= sendInterval) {
        lastSendTime = currentTime;

        client.println("test working");
        Serial.println("Sent: test working");
      }

      delay(10);
    }

    client.stop();
    Serial.println("Client disconnected.");
  }
}