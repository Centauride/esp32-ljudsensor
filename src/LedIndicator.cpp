#include "./include/LedIndicator.h"
#include "./include/Config.h"

void LedIndicator::begin() {
  _strip.begin();
  _strip.show();
  _strip.setBrightness(50);
}

void LedIndicator::update(uint32_t level) {
  _strip.setPixelColor(0, _strip.Color(0, 255, 0)); // always-on baseline

  _strip.setPixelColor(1, level >= THRESHOLD_QUIET_TO_NORMAL
    ? _strip.Color(255, 100, 0)
    : _strip.Color(0, 0, 0));

  _strip.setPixelColor(2, level >= THRESHOLD_NORMAL_TO_LOUD
    ? _strip.Color(255, 0, 0)
    : _strip.Color(0, 0, 0));


  _strip.show();
}
