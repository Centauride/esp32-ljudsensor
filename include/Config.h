#pragma once
#include <Arduino.h>

// ---------- WiFi ----------
// WIFI_SSID and WIFI_PASSWORD live in Secrets.h, which is gitignored.
// Each dev copies Secrets.h.example to Secrets.h and fills in their own.
#if !__has_include("Secrets.h")
  #error "Missing Secrets.h: copy include/Secrets.h.example to include/Secrets.h and fill in your WiFi credentials."
#endif
#include "Secrets.h"

#define WIFI_CONNECT_TIMEOUT_MS  20000  // boot-time attempt gives up after this long
#define WIFI_RETRY_INTERVAL_MS   30000  // after that, retry in the background this often
#define RETRY_BUTTON_PIN         9      // BOOT button on most ESP32-C3 boards: press to retry WiFi now

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

// ---------- Smoothing ----------
// Exponential moving average factor: lower = smoother/slower, higher = snappier/noisier
#define SMOOTHING_ALPHA   0.2

// ---------- LED colors (0xRRGGBB) ----------
#define COLOR_BASELINE   0x00FF00   // green
#define COLOR_NORMAL     0xFF6400   // orange
#define COLOR_LOUD       0xFF0000   // red
#define COLOR_OFF        0x000000   // off
#define COLOR_LOADING    0x9F2B68   // purple, used while connecting to WiFi

// ---------- Sound thresholds ----------
// These are now RUNTIME-adjustable (via the web UI), so they're plain
// mutable variables rather than #define constants. Defined in Config.cpp,
// with the values below used only as the starting defaults.
extern uint32_t thresholdQuietToNormal; // orange LED turns on above this
extern uint32_t thresholdNormalToLoud;  // red LED turns on above this