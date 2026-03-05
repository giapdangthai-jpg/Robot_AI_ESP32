#include "speaker_i2s.h"
#include "../config/pinmap.h"
#include <driver/i2s.h>

// Configure I2S_NUM_1 for MAX98357 amplifier (TX only, 16 kHz, 16-bit PCM)
void SpeakerI2S::init() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPK_BCLK,
        .ws_io_num = I2S_SPK_WS,
        .data_out_num = I2S_SPK_DATA,
        .data_in_num = -1            // TX only, no input pin
    };

    i2s_driver_install(I2S_NUM_1, &config, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &pin_config);
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
