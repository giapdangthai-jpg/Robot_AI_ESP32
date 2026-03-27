#include "mic_i2s.h"
#include "../config/pinmap.h"
#include <driver/i2s.h>
#include <Arduino.h>

#define I2S_MIC_PORT    I2S_NUM_0
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     512
#define MIC_SAMPLE_RATE 16000

void MicI2S::init() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = MIC_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = true   // APLL: clock jitter <10 ppm vs ~200 ppm — better audio quality for STT
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_BCLK,
        .ws_io_num = I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_DATA
    };

    esp_err_t err = i2s_driver_install(I2S_MIC_PORT, &config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_driver_install failed: 0x%x (%s)\n", err, esp_err_to_name(err));
        return;
    }
    err = i2s_set_pin(I2S_MIC_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[MIC] i2s_set_pin failed: 0x%x (%s)\n", err, esp_err_to_name(err));
    }
}
