#include "system.h"
#include "../network/wifi_mgr.h"
#include "../network/websocket_mgr.h"
#include "../audio/speaker_i2s.h"
#include "../audio/mic_i2s.h"
#include "../audio/ws_audio_play_task.h"
#include "../audio/mic_stream_task.h"
#include "../audio/mic_upload_task.h"
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"
#include <Arduino.h>

WifiMgr wifi;
WebSocketMgr wsmgr;

void System::init() {
    Serial.begin(DEBUG_BAUD);
    delay(500);
    Serial.println("System init...");
    if (!g_micBuf.init(8192, false)) {
        Serial.println("[SYS] Failed to init mic buffer");
    }
    MicI2S::init();
    SpeakerI2S::init();
    wifi.connect();
    wsmgr.init();       // g_spkBuf.init() được gọi bên trong
    wsmgr.startTask();
    startWsAudioPlayTask();
}

void System::startMicStream() {
    startMicTask();
    startMicUploadTask(&wsmgr);
}
