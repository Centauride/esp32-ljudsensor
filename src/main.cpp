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

// INMP441: tie L/R pin to GND -> mic outputs on the LEFT channel

#define I2S_PORT        I2S_NUM_0
#define SAMPLE_RATE     16000
#define I2S_READ_LEN    512          // samples per read (int32_t each)

// ---------- Sound thresholds ----------
#define THRESHOLD_QUIET_TO_NORMAL   2000    // below this -> yellow (silence/negligible)
#define THRESHOLD_NORMAL_TO_LOUD    30000   // above this -> red (too noisy)
// between the two -> orange (normal conversation)

int32_t i2s_buffer[I2S_READ_LEN];

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,   // TEST: try RIGHT if LEFT reads flat 0/1
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

  esp_err_t install_res = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  esp_err_t pin_res = i2s_set_pin(I2S_PORT, &pin_config);

  Serial.print("i2s_driver_install res=");
  Serial.println(install_res);
  Serial.print("i2s_set_pin res=");
  Serial.println(pin_res);
}

// Reads a block of samples and returns their RMS amplitude
uint32_t readSoundRMS() {
  size_t bytes_read = 0;
  esp_err_t res = i2s_read(I2S_PORT, i2s_buffer, sizeof(i2s_buffer), &bytes_read, portMAX_DELAY);

  // --- DEBUG: remove once things are working ---
  Serial.print("res=");
  Serial.print(res);
  Serial.print(" bytes_read=");
  Serial.print(bytes_read);
  Serial.print(" raw[0..3]=");
  for (int i = 0; i < 4; i++) {
    Serial.print(i2s_buffer[i]);
    Serial.print(" ");
  }
  Serial.println();
  // --- end debug ---

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

void setLedColor(uint32_t r, uint32_t g, uint32_t b) {
  strip.fill(strip.Color(r, g, b));
  strip.show();
}

void setup() {
  Serial.begin(115200);
  delay(1000); // give Serial a moment to connect before printing

  strip.begin();
  strip.show();
  strip.setBrightness(50);

  setupI2S();

  Serial.println("SETUP FINISHED");
}

void loop() {
  uint32_t level = readSoundRMS();

  Serial.print("level=");
  Serial.println(level);

  if (level < THRESHOLD_QUIET_TO_NORMAL) {
    setLedColor(255, 255, 0);   // yellow - silence / negligible
  } else if (level < THRESHOLD_NORMAL_TO_LOUD) {
    setLedColor(255, 100, 0);   // orange - normal conversation
  } else {
    setLedColor(255, 0, 0);     // red - too noisy
  }

  delay(300); // slower loop so Serial output is readable
}
