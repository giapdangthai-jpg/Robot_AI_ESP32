#include "ws_audio_play_task.h"
#include <Arduino.h>
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
static constexpr unsigned long SPK_TAIL_MS = 1500; // unmute mic after 1.5s silence — covers network jitter between TTS chunks
static constexpr unsigned long SPK_DONE_MS = 2000; // log "done" after 2s silence
// Pre-buffer: wait until 200ms of audio is queued before starting playback to prevent underrun
static constexpr size_t SPK_PREBUF_SAMPLES = 4410; // 200ms @ 22050 Hz
static uint32_t g_spkWriteFails = 0;
static uint32_t g_spkFramesPlayed = 0;
static unsigned long g_lastSpkWriteLogMs = 0;
static unsigned long g_lastSpkPopMs = 0;

static void wsAudioPlayTask(void* pv) {
    (void)pv;
    int16_t frame[AUDIO_FRAME_SAMPLES];
    bool buffering = true;  // true = waiting for pre-buffer to fill

    while (true) {
        // Wait for pre-buffer to fill before first frame of a new utterance
        if (buffering) {
            if (g_spkBuf.size() < SPK_PREBUF_SAMPLES) {
                vTaskDelay(pdMS_TO_TICKS(5));
                continue;
            }
            buffering = false;
        }

        if (g_spkBuf.pop(frame, AUDIO_FRAME_SAMPLES)) {
            g_lastSpkPopMs = millis();
            g_speakerActive = true;
            g_spkFramesPlayed++;
            if (!SpeakerI2S::write(frame, AUDIO_FRAME_BYTES)) {
                g_spkWriteFails++;
                unsigned long now = millis();
                if (now - g_lastSpkWriteLogMs >= 2000) {
                    g_lastSpkWriteLogMs = now;
                    Serial.printf("[SPK] write_fail=%lu\n", (unsigned long)g_spkWriteFails);
                }
            }
        } else {
            unsigned long elapsed = millis() - g_lastSpkPopMs;
            if (g_speakerActive && elapsed > SPK_TAIL_MS) {
                g_speakerActive = false;
            }
            if (g_spkFramesPlayed > 0 && elapsed > SPK_DONE_MS) {
                Serial.printf("[SPK] playback done: frames=%lu bytes=%lu fails=%lu\n",
                              (unsigned long)g_spkFramesPlayed,
                              (unsigned long)g_spkFramesPlayed * AUDIO_FRAME_BYTES,
                              (unsigned long)g_spkWriteFails);
                g_spkFramesPlayed = 0;
                g_spkWriteFails   = 0;
                buffering = true;  // re-arm pre-buffer for next utterance
            }
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

void startWsAudioPlayTask() {
    BaseType_t ok = xTaskCreatePinnedToCore(
        wsAudioPlayTask,
        "ws_audio_play",
        WS_AUDIO_TASK_STACK,
        nullptr,
        WS_AUDIO_TASK_PRIO,
        nullptr,
        WS_AUDIO_TASK_CORE
    );
    if (ok != pdPASS) {
        Serial.println("[SPK] Failed to create ws_audio_play task");
    }
}
