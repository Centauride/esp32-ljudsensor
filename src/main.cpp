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
  if (!settingsServer.begin(ledIndicator)) {
    Serial.println("Running WITHOUT web UI (WiFi unavailable). Thresholds use defaults.");
  }

  Serial.println("SETUP FINISHED");
}

void loop() {
  uint32_t level = audioSensor.update();

  ledIndicator.update(level);
  settingsServer.setCurrentLevel(level);
  settingsServer.handleClient(); // must be called often, keeps the web server responsive

  delay(50);
}