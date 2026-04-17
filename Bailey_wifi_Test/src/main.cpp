#include <arduino.h>
#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "bailey_phone";
const char* password = "abcdefgh";

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // Prints ESP32 IP address
}

void loop() {
  // Your code here
}
