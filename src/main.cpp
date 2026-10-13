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

  pinMode(RETRY_BUTTON_PIN, INPUT_PULLUP);  // button pulls the pin LOW when pressed

  ledIndicator.begin();
  audioSensor.begin();
  if (!settingsServer.begin(ledIndicator)) {
    Serial.println("Running WITHOUT web UI (WiFi unavailable). Thresholds use defaults.");
  }

  Serial.println("SETUP FINISHED");
}

void loop() {
  // Manual retry: act once per button press, not on every loop while held
  static bool wasPressed = false;
  bool pressed = digitalRead(RETRY_BUTTON_PIN) == LOW;
  if (pressed && !wasPressed) {
    settingsServer.retryNow();
  }
  wasPressed = pressed;

  settingsServer.maintain(); // background WiFi retry + starts the web server once connected

  uint32_t level = audioSensor.update();

  ledIndicator.update(level);
  settingsServer.setCurrentLevel(level);
  settingsServer.handleClient(); // must be called often, keeps the web server responsive

  delay(50);
}