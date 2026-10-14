#include <Arduino.h>
#include "Config.h"
#include "AudioSensor.h"
#include "LedIndicator.h"
#include "SettingsServer.h"
#include "WifiCredentials.h"
#include "WifiProvisioner.h"

AudioSensor audioSensor;
LedIndicator ledIndicator;
SettingsServer settingsServer;
WifiCredentials wifiCredentials;
WifiProvisioner wifiProvisioner;

// Opens the WiFi setup portal. If the user saves a network, the board restarts
// and connects with it. If nobody does within the timeout, this returns and
// the meter carries on without WiFi.
static void runSetupPortal(const String &prefillSsid, const String &message) {
  if (wifiProvisioner.run(ledIndicator, wifiCredentials, prefillSsid, message, SETUP_PORTAL_TIMEOUT_MS)) {
    ESP.restart();
  }
  Serial.println("Setup portal timed out. Running WITHOUT WiFi until a restart or a long BOOT press.");
}

// BOOT button: a short press retries the WiFi connection, holding it for
// LONG_PRESS_MS erases the saved network so the setup portal opens again.
static void handleButton() {
  static bool pressed = false;
  static unsigned long pressStartMs = 0;

  bool down = digitalRead(RETRY_BUTTON_PIN) == LOW;

  if (down && !pressed) {
    pressed = true;
    pressStartMs = millis();
  } else if (down && pressed && millis() - pressStartMs >= LONG_PRESS_MS) {
    Serial.println("Long press: saved WiFi erased. Let go of the button to restart.");
    wifiCredentials.clear();
    ledIndicator.flash(COLOR_LOUD, 3);
    // Restart only after the button is released: GPIO9 held low during a
    // reset puts the chip into upload mode instead of running the program.
    while (digitalRead(RETRY_BUTTON_PIN) == LOW) {
      delay(20);
    }
    ESP.restart();
  } else if (!down && pressed) {
    pressed = false;
    if (millis() - pressStartMs > 50) {
      settingsServer.retryNow();   // short press
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RETRY_BUTTON_PIN, INPUT_PULLUP);  // button pulls the pin LOW when pressed

  ledIndicator.begin();
  audioSensor.begin();

  String ssid, password;
  if (!wifiCredentials.load(ssid, password)) {
    Serial.println("No WiFi saved yet: opening the setup portal.");
    runSetupPortal("", "");
  } else if (settingsServer.begin(ledIndicator, ssid, password)) {
    wifiCredentials.markVerified();
  } else if (!wifiCredentials.isVerified()) {
    // A network that has never worked: most likely a typo in the password.
    runSetupPortal(ssid, "Couldn't connect to \"" + ssid + "\". Check the password and try again.");
  } else {
    Serial.println("Running WITHOUT web UI (WiFi unavailable). Retrying in the background.");
  }

  Serial.println("SETUP FINISHED");
}

void loop() {
  handleButton();

  settingsServer.maintain();  // background WiFi retry + starts the web server once connected
  static bool markedVerified = false;
  if (!markedVerified && settingsServer.isRunning()) {
    wifiCredentials.markVerified();  // also covers a connection that only worked in the background
    markedVerified = true;
  }

  uint32_t level = audioSensor.update();

  ledIndicator.update(level);
  settingsServer.setCurrentLevel(level);
  settingsServer.handleClient(); // must be called often, keeps the web server responsive

  delay(50);
}
