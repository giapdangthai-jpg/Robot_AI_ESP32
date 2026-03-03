#include "ws_audio_play_task.h"
#include "speaker_i2s.h"
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static constexpr uint32_t WS_AUDIO_TASK_STACK = 4096;
static constexpr UBaseType_t WS_AUDIO_TASK_PRIO = 1;
static constexpr BaseType_t WS_AUDIO_TASK_CORE = 0;

static void wsAudioPlayTask(void* pv) {
    (void)pv;
    int16_t frame[AUDIO_FRAME_SAMPLES];

    while (true) {
        if (g_spkBuf.pop(frame, AUDIO_FRAME_SAMPLES)) {
            SpeakerI2S::write(frame, AUDIO_FRAME_BYTES);
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

void startWsAudioPlayTask() {
    xTaskCreatePinnedToCore(
        wsAudioPlayTask,
        "ws_audio_play",
        WS_AUDIO_TASK_STACK,
        nullptr,
        WS_AUDIO_TASK_PRIO,
        nullptr,
        WS_AUDIO_TASK_CORE
    );
}
