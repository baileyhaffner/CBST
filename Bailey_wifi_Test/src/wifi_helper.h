#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

class WifiHelper {
public:
    WifiHelper(const char* ssid,
               const char* password,
               const char* targetIp,
               uint16_t targetPort);

    void begin();
    void sendTestPacket(const String& message);
    IPAddress localIP() const;

private:
    const char* _ssid;
    const char* _password;
    const char* _targetIp;
    uint16_t _targetPort;
    WiFiUDP _udp;
};

#endif