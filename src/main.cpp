#include <Arduino.h>
#include "Config.h"
#include "AudioSensor.h"
#include "LedIndicator.h"
#include "SettingsServer.h"

AudioSensor audioSensor;
LedIndicator ledIndicator;
SettingsServer settingsServer;

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledIndicator.begin();
  audioSensor.begin();
  settingsServer.begin(ledIndicator); // connects WiFi (shows loading animation), prints the IP to visit

  Serial.println("SETUP FINISHED");
}

void loop() {
  uint32_t level = audioSensor.update();

  ledIndicator.update(level);
  settingsServer.setCurrentLevel(level);
  settingsServer.handleClient(); // must be called often, keeps the web server responsive

  delay(50);
}
