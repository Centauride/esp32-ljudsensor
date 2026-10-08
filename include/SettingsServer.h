#pragma once
#include <WebServer.h>
#include "LedIndicator.h"

// Hosts a small web page (sliders + live level readout) on the ESP32's
// own WiFi connection, so thresholds can be tuned in real time from a
// phone or laptop browser instead of reflashing firmware each time.
class SettingsServer {
public:
  // ledIndicator is used to show a loading animation while connecting.
  void begin(LedIndicator &ledIndicator);
  void handleClient();    // call every loop() iteration (non-blocking)

  // main.cpp calls this each loop so the web page can show a live reading
  void setCurrentLevel(uint32_t level) { _currentLevel = level; }

private:
  WebServer _server{80};
  uint32_t _currentLevel = 0;

  void handleRoot();
  void handleStatus();
  void handleSet();
};
