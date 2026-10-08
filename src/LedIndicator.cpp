#include "LedIndicator.h"
#include "Config.h"

void LedIndicator::begin() {
  _strip.begin();
  _strip.show();
  _strip.setBrightness(50);
}

void LedIndicator::update(uint32_t level) {
  _strip.setPixelColor(0, COLOR_BASELINE);

  _strip.setPixelColor(1, level >= thresholdQuietToNormal ? COLOR_NORMAL : COLOR_OFF);

  _strip.setPixelColor(2, level >= thresholdNormalToLoud ? COLOR_LOUD : COLOR_OFF);

  _strip.show();
}

void LedIndicator::showLoadingStep() {
  // Single lit pixel that bounces back and forth across the strip (blue),
  // advancing one step each time this is called.
  _strip.clear();
  _strip.setPixelColor(_loadingStep, COLOR_LOADING);
  _strip.show();

  _loadingStep = (_loadingStep + 1) % NUM_PIXELS;
}
