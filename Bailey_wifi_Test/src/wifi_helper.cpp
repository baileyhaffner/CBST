#include "wifi_helper.h"

WifiHelper::WifiHelper(const char* ssid,
                       const char* password,
                       const char* targetIp,
                       uint16_t targetPort)
    : _ssid(ssid),
      _password(password),
      _targetIp(targetIp),
      _targetPort(targetPort) {}

void WifiHelper::begin() {
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(_ssid);

    // Keep this exactly like the version that worked
    WiFi.begin(_ssid, _password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    _udp.begin(_targetPort);
}

void WifiHelper::sendTestPacket(const String& message) {
    _udp.beginPacket(_targetIp, _targetPort);
    _udp.print(message);
    _udp.endPacket();

    Serial.print("Sent: ");
    Serial.println(message);
}

IPAddress WifiHelper::localIP() const {
    return WiFi.localIP();
}