#include "Config.h"

uint32_t thresholdQuietToNormal = DEFAULT_THRESHOLD_QUIET_TO_NORMAL;
uint32_t thresholdNormalToLoud  = DEFAULT_THRESHOLD_NORMAL_TO_LOUD;

void resetThresholdsToDefaults() {
  thresholdQuietToNormal = DEFAULT_THRESHOLD_QUIET_TO_NORMAL;
  thresholdNormalToLoud  = DEFAULT_THRESHOLD_NORMAL_TO_LOUD;
}

void enforceThresholdGap() {
  if (thresholdNormalToLoud < thresholdQuietToNormal + MIN_THRESHOLD_GAP) {
    thresholdNormalToLoud = thresholdQuietToNormal + MIN_THRESHOLD_GAP;
  }
}