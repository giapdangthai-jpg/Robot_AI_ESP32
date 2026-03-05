#include "mic_i2s.h"
#include "../config/pinmap.h"
#include <driver/i2s.h>

// Configure I2S_NUM_0 for INMP441 microphone (RX only, 16 kHz)
// INMP441 outputs 24-bit audio in 32-bit frames; upper 16 bits are extracted in mic_stream_task
void MicI2S::init() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // INMP441 requires 32-bit slot
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_BCLK,
        .ws_io_num = I2S_MIC_WS,
        .data_out_num = -1,          // RX only, no output pin
        .data_in_num = I2S_MIC_DATA
    };

    i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}
