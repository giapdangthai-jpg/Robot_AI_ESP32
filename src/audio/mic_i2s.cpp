#include "mic_i2s.h"
#include "../config/pinmap.h"
#include <driver/i2s.h>

// ── I2S config ─────────────────────────────────────────────────────────────
#define I2S_MIC_PORT        I2S_NUM_0
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     512

// ── Audio params ───────────────────────────────────────────────────────────
#define MIC_SAMPLE_RATE 16000   // INMP441 @ 16 kHz


// Configure I2S_NUM_0 for INMP441 microphone (RX slave, 16 kHz)
// Slave mode: BCLK/WS are driven by I2S_NUM_1 (speaker master) on shared pins
// INMP441 outputs 24-bit audio in 32-bit frames; upper 16 bits are extracted in mic_stream_task
void MicI2S::init() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX),
        .sample_rate = MIC_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // INMP441 requires 32-bit slot
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPK_MIC_BCLK,
        .ws_io_num = I2S_SPK_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,          // RX only, no output pin
        .data_in_num = I2S_MIC_DATA
    };

    ESP_ERROR_CHECK(i2s_driver_install(I2S_MIC_PORT, &config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_MIC_PORT, &pin_config));
}
