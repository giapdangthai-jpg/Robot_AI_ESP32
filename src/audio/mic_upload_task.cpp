#include "mic_upload_task.h"
#include <Arduino.h>
#include "vad.h"
#include "audio_preproc.h"
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

static constexpr uint32_t    MIC_UPLOAD_STACK = 3072;
static constexpr UBaseType_t MIC_UPLOAD_PRIO  = 1;
static constexpr BaseType_t  MIC_UPLOAD_CORE  = 0;

// ── Gate parameters ───────────────────────────────────────────────────────────
// The client VAD is a bandwidth gate, NOT a speech detector.
// Silero VAD on the server makes the real speech/non-speech decision.
//
// GATE_OPEN_FRAMES:  once a frame is above threshold, keep streaming for this
//                    many additional frames — ensures Silero gets enough context
//                    before and after the loud event.
// GATE_CALIB_FRAMES: update noise floor for this many frames on connect before
//                    opening the gate, so the threshold starts near ambient.
static constexpr uint8_t  GATE_OPEN_FRAMES  = 50;  // 50 × 20ms = 1s hold-open after last loud frame
static constexpr uint16_t GATE_CALIB_FRAMES = 50;  // 50 × 20ms = 1s calibration on connect

static void micUploadTask(void* pv) {
    WebSocketMgr* mgr = static_cast<WebSocketMgr*>(pv);
    int16_t frame[AUDIO_FRAME_SAMPLES];

    uint8_t  holdCount   = 0;     // frames remaining in hold-open window
    uint16_t calibFrames = 0;     // calibration counter

    uint32_t     framesSent    = 0;
    uint32_t     framesDropped = 0;
    unsigned long lastLogMs    = 0;

    while (true) {
        if (!g_micBuf.pop(frame, AUDIO_FRAME_SAMPLES)) {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        // Echo suppression: mute while speaker is playing.
        // Also reset filter state so HPF/PE transients don't contaminate
        // the first frames after the speaker stops.
        if (g_speakerActive) {
            holdCount   = 0;
            calibFrames = 0;
            AudioPreproc::reset();
            VAD::updateNoiseFloor(frame, AUDIO_FRAME_SAMPLES);  // keep floor updated
            framesDropped++;
            continue;
        }

        if (!mgr->isConnected()) {
            holdCount   = 0;
            calibFrames = 0;
            framesDropped++;
            continue;
        }

        // Calibration: update noise floor for 1s after connect before gating
        if (calibFrames < GATE_CALIB_FRAMES) {
            VAD::updateNoiseFloor(frame, AUDIO_FRAME_SAMPLES);
            calibFrames++;
            continue;
        }

        bool loud = VAD::isSpeech(frame, AUDIO_FRAME_SAMPLES);

        if (loud) {
            holdCount = GATE_OPEN_FRAMES;  // reset hold window
        } else if (holdCount > 0) {
            holdCount--;                   // count down hold window
        } else {
            // True silence: update noise floor and do not stream
            VAD::updateNoiseFloor(frame, AUDIO_FRAME_SAMPLES);
            framesDropped++;
            continue;
        }

        // Gate is open — stream frame to server (Silero VAD decides if it's speech)
        if (mgr->sendBinary((const uint8_t*)frame, AUDIO_FRAME_BYTES)) {
            framesSent++;
        } else {
            framesDropped++;
        }

        unsigned long now = millis();
        if (framesDropped > 0 && now - lastLogMs >= 5000) {
            lastLogMs = now;
            Serial.printf("[MIC][UP] sent=%lu dropped=%lu\n",
                          (unsigned long)framesSent,
                          (unsigned long)framesDropped);
        }
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
