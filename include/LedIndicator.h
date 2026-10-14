#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"

// Wraps the 3-LED NeoPixel bar: LED0 = baseline green, LED1 = orange
// once sound crosses the "normal" threshold, LED2 = red once it
// crosses the "loud" threshold.
class LedIndicator {
public:
  void begin();
  void update(uint32_t level);

  // Call repeatedly (e.g. once per WiFi connection retry) to animate a
  // simple "loading" chase across the strip while connecting.
  void showLoadingStep();

  // Slow "breathing" of all LEDs while the WiFi setup portal is open.
  // Call it often (every few tens of ms) from the portal's loop.
  void showSetupPulse();

  // Blinks all LEDs in one color, e.g. to confirm an action.
  void flash(uint32_t color, uint8_t times);

private:
  Adafruit_NeoPixel _strip{NUM_PIXELS, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800};
  uint8_t _loadingStep = 0;
};
