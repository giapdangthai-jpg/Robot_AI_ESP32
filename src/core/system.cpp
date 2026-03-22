#include "system.h"
#include "../network/wifi_mgr.h"
#include "../network/websocket_mgr.h"
#include "../audio/speaker_i2s.h"
#include "../audio/mic_i2s.h"
#include "../audio/ws_audio_play_task.h"
#include "../audio/mic_stream_task.h"
#include "../audio/mic_upload_task.h"
#include "../utils/audio_buffer.h"
#include "../utils/rgb_led.h"
#include "../config/pinmap.h"
#include "../input/touch_task.h"
#include <Arduino.h>

WifiMgr wifi;
WebSocketMgr wsmgr;

void System::init() {
    Serial.begin(DEBUG_BAUD);
    delay(500);
    RgbLed::init();
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);    // prepare for runtime hold-detection
    Serial.println("System init...");

    if (!g_micBuf.init(8192, false)) {
        Serial.println("[SYS] Failed to init mic buffer");
    }
    if (!g_spkBuf.init(32768, true)) {  // PSRAM: large buffer to absorb TTS bursts
        Serial.println("[SYS] Failed to init speaker buffer");
    }

    // WiFi must init before I2S to avoid GDMA channel exhaustion
    wifi.connect();

    MicI2S::init();       // I2S_NUM_0: INMP441 microphone — init before speaker (GDMA ordering)
    SpeakerI2S::init();   // I2S_NUM_1: MAX98357 amplifier

    wsmgr.init();
    wsmgr.startTask();        // FreeRTOS task: WS loop + send queue on Core 0
    startWsAudioPlayTask();   // pops g_spkBuf and writes to I2S speaker
    startMicStream();         // captures I2S mic and uploads via WebSocket
    startTouchTask();         // Touch task
}

void System::startMicStream() {
    static bool started = false;
    if (started) {
        Serial.println("[SYS] Mic stream already started");
        return;
    }
    startMicTask();              // Core 1: reads I2S, pushes to g_micBuf
    startMicUploadTask(&wsmgr);  // Core 0: VAD + sends frames over WebSocket
    started = true;
}
