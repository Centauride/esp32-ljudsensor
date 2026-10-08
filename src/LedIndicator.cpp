#include "./include/LedIndicator.h"
#include "./include/Config.h"

void LedIndicator::begin() {
  _strip.begin();
  _strip.show();
  _strip.setBrightness(50);
}

void LedIndicator::update(uint32_t level) {
  _strip.setPixelColor(0, COLOR_BASELINE);

  _strip.setPixelColor(1, level >= THRESHOLD_QUIET_TO_NORMAL ? COLOR_NORMAL : COLOR_OFF);

  _strip.setPixelColor(2, level >= THRESHOLD_NORMAL_TO_LOUD ? COLOR_LOUD : COLOR_OFF);

  _strip.show();
}