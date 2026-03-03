#include "mic_stream_task.h"
#include <driver/i2s.h>
#include "../config/pinmap.h"
#include "../utils/audio_buffer.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static constexpr uint32_t MIC_TASK_STACK = 4096;
static constexpr UBaseType_t MIC_TASK_PRIO = 2;
static constexpr BaseType_t MIC_TASK_CORE = 1;

static void micTask(void* pv) {
    (void)pv;
    // INMP441 dùng 32-bit mode: mỗi sample là int32_t, MSB 16-bit là audio thực
    int32_t raw[AUDIO_FRAME_SAMPLES];
    int16_t frame[AUDIO_FRAME_SAMPLES];
    size_t bytesRead;

    while (true) {
        i2s_read(I2S_NUM_0,
                 raw,
                 sizeof(raw),
                 &bytesRead,
                 portMAX_DELAY);

        size_t samplesRead = bytesRead / sizeof(int32_t);
        if (samplesRead == AUDIO_FRAME_SAMPLES) {
            for (size_t i = 0; i < samplesRead; i++) {
                frame[i] = (int16_t)(raw[i] >> 16);
            }
            g_micBuf.push(frame, AUDIO_FRAME_SAMPLES);
        }
    }
}

void startMicTask() {
    xTaskCreatePinnedToCore(
        micTask,
        "mic_task",
        MIC_TASK_STACK,
        nullptr,
        MIC_TASK_PRIO,
        nullptr,
        MIC_TASK_CORE
    );
}
