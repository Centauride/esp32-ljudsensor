#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "driver/i2s.h"

// ---------- NeoPixel setup ----------
#define LED_STRIP_PIN  5
#define NUM_PIXELS     3

Adafruit_NeoPixel strip(NUM_PIXELS, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);

// ---------- I2S / INMP441 pins ----------
#define I2S_BCLK_PIN   2   // SCK  (bit clock)
#define I2S_WS_PIN     4   // WS   (word select / L-R clock)
#define I2S_DIN_PIN    3   // SD   (data out from mic into ESP32 RX)

// This specific INMP441 board outputs on the RIGHT channel with L/R tied to GND
#define I2S_PORT        I2S_NUM_0
#define SAMPLE_RATE     16000
#define I2S_READ_LEN    512          // samples per read (int32_t each)

// ---------- Sound thresholds (PLACEHOLDER - recalibrate after this test) ----------
#define THRESHOLD_QUIET_TO_NORMAL   40000    // below this -> yellow (silence/negligible)
#define THRESHOLD_NORMAL_TO_LOUD    90000    // above this -> red (too noisy)
// between the two -> orange (normal conversation)

// Smoothing factor: lower = smoother/slower to react, higher = snappier/noisier
#define SMOOTHING_ALPHA   0.2

int32_t i2s_buffer[I2S_READ_LEN];
double smoothed_level = 0;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,   // confirmed working channel
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_READ_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_DIN_PIN
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

// Reads a block of samples and returns their raw RMS amplitude (no smoothing)
uint32_t readRawRMS() {
  size_t bytes_read = 0;
  esp_err_t res = i2s_read(I2S_PORT, i2s_buffer, sizeof(i2s_buffer), &bytes_read, portMAX_DELAY);
  if (res != ESP_OK || bytes_read == 0) {
    return 0;
  }

  int samples_read = bytes_read / sizeof(int32_t);
  int64_t sum_sq = 0;

  for (int i = 0; i < samples_read; i++) {
    int32_t sample = i2s_buffer[i] >> 8;
    sum_sq += (int64_t)sample * (int64_t)sample;
  }

  double mean_sq = (double)sum_sq / samples_read;
  return (uint32_t)sqrt(mean_sq);
}

void updateLedBar(uint32_t level) {
  // LED 0: green, always on as a baseline/power indicator
  strip.setPixelColor(0, strip.Color(0, 255, 0));

  // LED 1: orange, turns on once we're at normal conversation level or louder
  if (level >= THRESHOLD_QUIET_TO_NORMAL) {
    strip.setPixelColor(1, strip.Color(255, 100, 0));
  } else {
    strip.setPixelColor(1, strip.Color(0, 0, 0));
  }

  // LED 2: red, turns on once it's too noisy
  if (level >= THRESHOLD_NORMAL_TO_LOUD) {
    strip.setPixelColor(2, strip.Color(255, 0, 0));
  } else {
    strip.setPixelColor(2, strip.Color(0, 0, 0));
  }

  strip.show();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  strip.begin();
  strip.show();
  strip.setBrightness(50);

  setupI2S();

  Serial.println("SETUP FINISHED");
}

void loop() {
  uint32_t raw_level = readRawRMS();

  // Exponential moving average smoothing
  smoothed_level = (SMOOTHING_ALPHA * raw_level) + ((1 - SMOOTHING_ALPHA) * smoothed_level);
  uint32_t level = (uint32_t)smoothed_level;

  Serial.print("raw=");
  Serial.print(raw_level);
  Serial.print(" smoothed=");
  Serial.println(level);

  updateLedBar(level);

  delay(100);
}
