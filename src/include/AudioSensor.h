#pragma once
#include <Arduino.h>

// Wraps the INMP441 I2S microphone: handles driver setup, raw sampling,
// and exposes a single smoothed sound-level reading.
class AudioSensor {
public:
  void begin();

  // Reads one block from the mic, updates the internal smoothed level,
  // and returns it. Call this once per loop iteration.
  uint32_t update();

  // Last smoothed level without triggering a new read.
  uint32_t getSmoothedLevel() const { return (uint32_t)_smoothedLevel; }

private:
  uint32_t readRawRMS();

  int32_t _buffer[512]; // matches I2S_READ_LEN in Config.h
  double _smoothedLevel = 0;
};
