#include "./include/AudioSensor.h"
#include "./include/Config.h"
#include "driver/i2s.h"

#define I2S_PORT I2S_NUM_0

void AudioSensor::begin() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
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

uint32_t AudioSensor::readRawRMS() {
  size_t bytes_read = 0;
  esp_err_t res = i2s_read(I2S_PORT, _buffer, sizeof(_buffer), &bytes_read, portMAX_DELAY);
  if (res != ESP_OK || bytes_read == 0) {
    return 0;
  }

  int samples_read = bytes_read / sizeof(int32_t);
  int64_t sum_sq = 0;

  for (int i = 0; i < samples_read; i++) {
    int32_t sample = _buffer[i] >> 8;
    sum_sq += (int64_t)sample * (int64_t)sample;
  }

  double mean_sq = (double)sum_sq / samples_read;
  return (uint32_t)sqrt(mean_sq);
}

uint32_t AudioSensor::update() {
  uint32_t raw = readRawRMS();
  _smoothedLevel = (SMOOTHING_ALPHA * raw) + ((1 - SMOOTHING_ALPHA) * _smoothedLevel);
  return (uint32_t)_smoothedLevel;
}
