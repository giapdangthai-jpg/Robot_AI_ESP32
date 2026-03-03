#include "mic_upload_task.h"
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static constexpr uint32_t MIC_UPLOAD_STACK = 4096;
static constexpr UBaseType_t MIC_UPLOAD_PRIO = 1;
static constexpr BaseType_t MIC_UPLOAD_CORE = 0;

static void micUploadTask(void* pv) {
    WebSocketMgr* mgr = static_cast<WebSocketMgr*>(pv);
    int16_t frame[AUDIO_FRAME_SAMPLES];

    while (true) {
        if (mgr->isConnected() && g_micBuf.pop(frame, AUDIO_FRAME_SAMPLES)) {
            mgr->sendBinary((const uint8_t*)frame, AUDIO_FRAME_BYTES);
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

void startMicUploadTask(WebSocketMgr* mgr) {
    xTaskCreatePinnedToCore(
        micUploadTask,
        "mic_upload",
        MIC_UPLOAD_STACK,
        mgr,
        MIC_UPLOAD_PRIO,
        nullptr,
        MIC_UPLOAD_CORE
    );
}
