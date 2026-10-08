#pragma once

// ---------- NeoPixel ----------
#define LED_STRIP_PIN  5
#define NUM_PIXELS     3

// ---------- I2S / INMP441 ----------
#define I2S_BCLK_PIN   2   // SCK  (bit clock)
#define I2S_WS_PIN     4   // WS   (word select / L-R clock)
#define I2S_DIN_PIN    3   // SD   (data out from mic into ESP32 RX)
// This specific INMP441 board outputs on the RIGHT channel with L/R tied to GND

#define SAMPLE_RATE     16000
#define I2S_READ_LEN    512          // samples per read (int32_t each)

// ---------- Sound thresholds (tune these after calibration) ----------
#define THRESHOLD_QUIET_TO_NORMAL   40000    // orange LED turns on above this
#define THRESHOLD_NORMAL_TO_LOUD    90000    // red LED turns on above this

// ---------- Smoothing ----------
// Exponential moving average factor: lower = smoother/slower, higher = snappier/noisier
#define SMOOTHING_ALPHA   0.2

// ---------- LED colors (0xRRGGBB) ----------
// #define COLOR_BASELINE   0x00FF00   // green
#define COLOR_BASELINE   0x9F2B68   // purple
#define COLOR_NORMAL     0xFF6400   // orange
#define COLOR_LOUD       0xFF0000   // red
#define COLOR_OFF        0x000000   // off