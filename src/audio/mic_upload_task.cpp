#include "mic_upload_task.h"
#include <Arduino.h>
#include <string.h>
#include "vad.h"
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static constexpr uint32_t MIC_UPLOAD_STACK = 4096;
static constexpr UBaseType_t MIC_UPLOAD_PRIO = 1;
static constexpr BaseType_t MIC_UPLOAD_CORE = 0;

static constexpr uint8_t VAD_ONSET_FRAMES   = 2;    // 2 × 20ms = 40ms above threshold → speech start
static constexpr uint8_t VAD_SILENCE_FRAMES = 20;   // 20 × 20ms = 400ms below threshold → speech end

static uint32_t g_uplinkFramesSent    = 0;
static uint32_t g_uplinkFramesDropped = 0;
static unsigned long g_lastUplinkLogMs = 0;

enum class VadState : uint8_t { IDLE, SPEAKING };

static void recordUplinkResult(bool ok) {
    if (ok) {
        g_uplinkFramesSent++;
    } else {
        g_uplinkFramesDropped++;
    }
}

static bool sendFrame(WebSocketMgr* mgr, const int16_t* frame) {
    bool ok = mgr->sendBinary((const uint8_t*)frame, AUDIO_FRAME_BYTES);
    recordUplinkResult(ok);
    return ok;
}

static void logUplinkStats() {
    unsigned long now = millis();
    if (now - g_lastUplinkLogMs < 2000) {
        return;
    }
    g_lastUplinkLogMs = now;
    if (g_uplinkFramesDropped > 0) {
        Serial.printf("[MIC][UP] sent=%lu dropped=%lu\n",
                      (unsigned long)g_uplinkFramesSent,
                      (unsigned long)g_uplinkFramesDropped);
    }
}

// VAD state machine + uplink task
// IDLE   → accumulates preRoll frames; transitions to SPEAKING after VAD_ONSET_FRAMES loud frames
// SPEAKING → sends every frame; transitions back to IDLE after VAD_SILENCE_FRAMES quiet frames
//            then sends {"type":"audio_end"} to trigger server transcription
static void micUploadTask(void* pv) {
    WebSocketMgr* mgr = static_cast<WebSocketMgr*>(pv);
    int16_t frame[AUDIO_FRAME_SAMPLES];
    int16_t preRoll[VAD_ONSET_FRAMES][AUDIO_FRAME_SAMPLES];  // sliding window before speech onset

    VadState vadState  = VadState::IDLE;
    uint8_t onsetCount   = 0;
    uint8_t silenceCount = 0;
    uint8_t preRollCount = 0;

    while (true) {
        if (!g_micBuf.pop(frame, AUDIO_FRAME_SAMPLES)) {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        // Drain buffer but discard frames while speaker is playing (echo suppression)
        if (g_speakerActive) {
            vadState     = VadState::IDLE;
            onsetCount   = 0;
            silenceCount = 0;
            preRollCount = 0;
            continue;
        }

        if (!mgr->isConnected()) {
            vadState     = VadState::IDLE;
            onsetCount   = 0;
            silenceCount = 0;
            preRollCount = 0;
            continue;
        }

        bool loud = VAD::isSpeech(frame, AUDIO_FRAME_SAMPLES);

        if (vadState == VadState::IDLE) {
            if (loud) {
                // Buffer loud frames in a sliding preRoll window
                if (preRollCount < VAD_ONSET_FRAMES) {
                    memcpy(preRoll[preRollCount], frame, AUDIO_FRAME_BYTES);
                    preRollCount++;
                } else {
                    memmove(preRoll, preRoll + 1, (VAD_ONSET_FRAMES - 1) * AUDIO_FRAME_BYTES);
                    memcpy(preRoll[VAD_ONSET_FRAMES - 1], frame, AUDIO_FRAME_BYTES);
                }

                if (++onsetCount >= VAD_ONSET_FRAMES) {
                    vadState     = VadState::SPEAKING;
                    onsetCount   = 0;
                    silenceCount = 0;
                    Serial.println("[VAD] speech start");

                    // Flush preRoll so server receives audio from before onset
                    for (uint8_t i = 0; i < preRollCount; ++i) {
                        sendFrame(mgr, preRoll[i]);
                    }
                    preRollCount = 0;
                }
            } else {
                onsetCount = 0;
                preRollCount = 0;
            }
        } else { // SPEAKING: send every frame; count trailing silence
            bool sent = sendFrame(mgr, frame);
            if (!sent) {
                // WS queue full — abort session to avoid flooding dropped frames
                Serial.println("[VAD] send fail — aborting speech session");
                vadState     = VadState::IDLE;
                silenceCount = 0;
                preRollCount = 0;
            } else if (!loud) {
                if (++silenceCount >= VAD_SILENCE_FRAMES) {
                    vadState     = VadState::IDLE;
                    silenceCount = 0;
                    // Signal server to run transcription immediately
                    if (!mgr->sendText("{\"type\":\"audio_end\"}")) {
                        Serial.println("[VAD] failed to queue audio_end");
                    } else {
                        Serial.println("[VAD] speech end -> audio_end sent");
                    }
                }
            } else {
                silenceCount = 0;
            }
        }

        logUplinkStats();
    }
}

void startMicUploadTask(WebSocketMgr* mgr) {
    BaseType_t ok = xTaskCreatePinnedToCore(
        micUploadTask,
        "mic_upload",
        MIC_UPLOAD_STACK,
        mgr,
        MIC_UPLOAD_PRIO,
        nullptr,
        MIC_UPLOAD_CORE
    );
    if (ok != pdPASS) {
        Serial.println("[MIC][UP] Failed to create mic_upload task");
    }
}
