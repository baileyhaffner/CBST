#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Testwifi";
const char* password = "12345678";

WebServer server(80);

int counter = 0;

void handleRoot() {
  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  page += "<meta http-equiv='refresh' content='2'>";
  page += "<title>ESP32 Data</title></head><body>";
  page += "<h1>ESP32 is working</h1>";
  page += "<p>Counter: " + String(counter) + "</p>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();

  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  counter++;
  delay(1000);
}