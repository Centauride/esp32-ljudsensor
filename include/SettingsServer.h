#pragma once
#include <WebServer.h>
#include "LedIndicator.h"

// Hosts a small web page (sliders + live level readout) on the ESP32's
// own WiFi connection, so thresholds can be tuned in real time from a
// phone or laptop browser instead of reflashing firmware each time.
class SettingsServer {
public:
  // ledIndicator is used to show a loading animation while connecting.
  // Returns true if WiFi connected and the server started, false on timeout.
  bool begin(LedIndicator &ledIndicator);
  void handleClient();    // call every loop() iteration (non-blocking)

  // main.cpp calls this each loop so the web page can show a live reading
  void setCurrentLevel(uint32_t level) { _currentLevel = level; }

private:
  WebServer _server{80};
  uint32_t _currentLevel = 0;
  bool _running = false;

  void handleRoot();
  void handleStatus();
  void handleSet();
};