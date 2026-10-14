#include "LedIndicator.h"
#include "Config.h"

// Dims a 0xRRGGBB color: level 255 = unchanged, 0 = off.
static uint32_t scaleColor(uint32_t color, uint8_t level) {
  uint32_t r = ((color >> 16) & 0xFF) * level / 255;
  uint32_t g = ((color >> 8) & 0xFF) * level / 255;
  uint32_t b = (color & 0xFF) * level / 255;
  return (r << 16) | (g << 8) | b;
}

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
  // Single lit pixel that moves across the strip, advancing one step each
  // time this is called.
  _strip.clear();
  _strip.setPixelColor(_loadingStep, COLOR_LOADING);
  _strip.show();

  _loadingStep = (_loadingStep + 1) % NUM_PIXELS;
}

void LedIndicator::showSetupPulse() {
  float phase = (sin(millis() / 350.0f) + 1.0f) / 2.0f;    // 0..1, about one breath every 2 s
  uint8_t level = 15 + (uint8_t)(phase * 240);
  _strip.fill(scaleColor(COLOR_SETUP, level));
  _strip.show();
}

void LedIndicator::flash(uint32_t color, uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    _strip.fill(color);
    _strip.show();
    delay(150);
    _strip.clear();
    _strip.show();
    delay(150);
  }
}
