#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace WifiServerModule {
  void begin();
  void handleClient();
  bool clientConnected();
  void sendLine(const char* line);
  void sendHeader();
  IPAddress apIP();
}