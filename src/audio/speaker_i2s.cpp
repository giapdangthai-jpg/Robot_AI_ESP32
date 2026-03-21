#include "speaker_i2s.h"
#include "../config/pinmap.h"
#include <driver/i2s.h>
#include <Arduino.h>

// ── I2S config ─────────────────────────────────────────────────────────────
#define I2S_SPK_PORT    I2S_NUM_1
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     512

// ── Audio params ───────────────────────────────────────────────────────────
#define SAMPLE_RATE 12000       // Điều chỉnh nhanh chậm của việc phát âm thanh

// Configure I2S_NUM_1 for MAX98357 amplifier (TX only, 16 kHz, 16-bit PCM)
void SpeakerI2S::init() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear   = true,
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPK_BCLK,
        .ws_io_num = I2S_SPK_WS,
        .data_out_num = I2S_SPK_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE            // TX only, no input pin
    };

    esp_err_t err = i2s_driver_install(I2S_SPK_PORT, &config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[SPK] i2s_driver_install failed: 0x%x (%s)\n", err, esp_err_to_name(err));
        Serial.flush();
        return;
    }
    err = i2s_set_pin(I2S_SPK_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[SPK] i2s_set_pin failed: 0x%x (%s)\n", err, esp_err_to_name(err));
        Serial.flush();
    }

    // Enable MAX98357 amplifier: SD pin HIGH = active (LOW = shutdown)
    pinMode(I2S_SPK_SD, OUTPUT);
    digitalWrite(I2S_SPK_SD, HIGH);
}

// Write one frame of 16-bit PCM samples to the speaker
// Returns false if write fails or not all bytes were consumed within 100ms timeout
bool SpeakerI2S::write(const int16_t* samples, size_t bytes) {
    if (samples == nullptr || bytes == 0) {
        return false;
    }

    size_t written = 0;
    esp_err_t err = i2s_write(
        I2S_NUM_1,
        samples,
        bytes,
        &written,
        pdMS_TO_TICKS(100)  // non-blocking with timeout to avoid stalling the task
    );

    return (err == ESP_OK) && (written == bytes);
}
