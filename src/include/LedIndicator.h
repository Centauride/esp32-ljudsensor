#pragma once
#include "./include/Config.h"
#include <Adafruit_NeoPixel.h>

// Wraps the 3-LED NeoPixel bar: LED0 = baseline green, LED1 = orange
// once sound crosses the "normal" threshold, LED2 = red once it
// crosses the "loud" threshold.
class LedIndicator {
public:
  void begin();
  void update(uint32_t level);

private:
  Adafruit_NeoPixel _strip{NUM_PIXELS, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800};
};
