#include "wifi_server.h"

namespace WifiServerModule {

static const char* AP_SSID = "BAILEY_WIFI_TEST";
static const char* AP_PASSWORD = "123456789";   // must be at least 8 chars
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GATEWAY(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);
static const uint16_t TCP_PORT = 5000;

static WiFiServer server(TCP_PORT);
static WiFiClient client;

void begin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.begin();
  server.setNoDelay(true);

  Serial.println();
  Serial.println("WiFi access point started");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("TCP server listening on ");
  Serial.print(WiFi.softAPIP());
  Serial.print(":");
  Serial.println(TCP_PORT);
}

void handleClient() {
  if (client && client.connected()) {
    return;
  }

  WiFiClient newClient = server.available();
  if (newClient) {
    client = newClient;
    client.setNoDelay(true);

    Serial.println();
    Serial.println("TCP client connected");

    sendHeader();
  }
}

bool clientConnected() {
  return client && client.connected();
}

void sendLine(const char* line) {
  if (!clientConnected()) {
    return;
  }
  client.println(line);
}

void sendHeader() {
  sendLine("ax,ay,az,gx,gy,gz,temp");
}

IPAddress apIP() {
  return WiFi.softAPIP();
}

}  // namespace WifiServerModule