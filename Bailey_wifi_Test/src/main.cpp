#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "Bailey’s iPhone 14 pro";
const char* password = "truck101";

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("Starting scan...");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(500);

  int n = WiFi.scanNetworks();
  Serial.print("Networks found: ");
  Serial.println(n);

  for (int i = 0; i < n; i++) {
    Serial.print(i + 1);
    Serial.print(": '");
    Serial.print(WiFi.SSID(i));
    Serial.print("' RSSI=");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" Ch=");
    Serial.println(WiFi.channel(i));
  }

  WiFi.scanDelete();

  Serial.println();
  Serial.print("Connecting to '");
  Serial.print(ssid);
  Serial.println("'...");

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  Serial.print("Final status: ");
  Serial.println(WiFi.status());

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("FAILED TO CONNECT");
  }
}

void loop() {
  delay(1000);
}