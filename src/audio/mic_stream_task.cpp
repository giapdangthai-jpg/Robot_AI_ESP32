#include "mic_stream_task.h"
#include <Arduino.h>
#include <driver/i2s.h>
#include "../config/pinmap.h"
#include "../utils/audio_buffer.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

// Task pinned to Core 1 (separate from WS/audio tasks on Core 0) for consistent timing
static constexpr uint32_t    MIC_TASK_STACK = 4096;
static constexpr UBaseType_t MIC_TASK_PRIO  = 2;     // higher than upload task to avoid buffer starvation
static constexpr BaseType_t  MIC_TASK_CORE  = 1;

static uint32_t      g_micFramesDropped  = 0;
static unsigned long g_lastMicDropLogMs  = 0;

// Continuously reads raw I2S frames from INMP441, converts to 16-bit PCM,
// and pushes into g_micBuf for consumption by mic_upload_task
static void micTask(void* pv) {
    (void)pv;
    // INMP441 uses 32-bit I2S mode; actual audio is in the upper 16 bits of each int32_t
    int32_t raw[AUDIO_FRAME_SAMPLES];
    int16_t frame[AUDIO_FRAME_SAMPLES];
    size_t bytesRead;

    while (true) {
        // Blocking read — waits until DMA has a full frame (AUDIO_FRAME_SAMPLES × 4 bytes)
        i2s_read(I2S_NUM_0,
                 raw,
                 sizeof(raw),
                 &bytesRead,
                 portMAX_DELAY);

        size_t samplesRead = bytesRead / sizeof(int32_t);
        if (samplesRead == AUDIO_FRAME_SAMPLES) {
            // Shift right 16 bits to extract the 16-bit audio from 32-bit INMP441 frame
            for (size_t i = 0; i < samplesRead; i++) {
                frame[i] = (int16_t)(raw[i] >> 16);
            }
            // Drop frame if buffer is full; log at most every 2s to avoid Serial flood
            if (!g_micBuf.push(frame, AUDIO_FRAME_SAMPLES)) {
                g_micFramesDropped++;
                unsigned long now = millis();
                if (now - g_lastMicDropLogMs >= 2000) {
                    g_lastMicDropLogMs = now;
                    Serial.printf("[MIC][CAP] dropped=%lu\n", (unsigned long)g_micFramesDropped);
                }
            }
        }
    }
}

// Create and pin micTask to Core 1
void startMicTask() {
    BaseType_t ok = xTaskCreatePinnedToCore(
        micTask,
        "mic_task",
        MIC_TASK_STACK,
        nullptr,
        MIC_TASK_PRIO,
        nullptr,
        MIC_TASK_CORE
    );
    if (ok != pdPASS) {
        Serial.println("[MIC][CAP] Failed to create mic_task");
    }
}
