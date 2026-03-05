#include "websocket_mgr.h"
#include <WebSocketsClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../ai/ai_bridge.h"
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"
#include "../test/hello_from_gabi_pcm.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
}

// Send queue: all WS writes go through a FreeRTOS queue so that
// ISR/tasks on any core can enqueue safely without calling WebSocketsClient directly
#define WS_STACK_SIZE         8192
#define WS_QUEUE_LEN          10    // max messages waiting to be sent
#define WS_MSG_SIZE           768   // max payload bytes per message (covers one audio frame)
#define WS_QUEUE_SEND_WAIT_MS 30    // max time to wait for queue space before dropping

enum class WsMsgType : uint8_t {
    TEXT   = 0,
    BINARY = 1
};

typedef struct {
    WsMsgType type;
    size_t    len;
    uint8_t   data[WS_MSG_SIZE];
} WsMessage;

// Singleton pointer used by the static webSocketEvent callback to reach the instance
static WebSocketMgr* g_instance = nullptr;
static uint32_t g_audioFramesRx = 0;
static uint32_t g_audioFramesDropped = 0;
static uint32_t g_audioSamplesDropped = 0;
static uint32_t g_audioBytesRx = 0;
static unsigned long g_lastAudioDropLogMs = 0;
static unsigned long g_lastAudioRxLogMs = 0;
// Initialize WebSocket client and speaker buffer (32768 samples in PSRAM)
// Heartbeat: ping every 30s, pong timeout 5s, disconnect after 2 missed pongs
void WebSocketMgr::init() {
    g_instance = this;

    if (!g_spkBuf.init(32768, true)) {  // large buffer in PSRAM to absorb TTS bursts
        Serial.println("[WS] Failed to init speaker buffer");
    }
    _ws.begin(WS_SERVER, WS_PORT, WS_PATH);
    _ws.onEvent(webSocketEvent);
    _ws.setReconnectInterval(RECONNECT_INTERVAL);
    _ws.enableHeartbeat(30000, 5000, 2);
    Serial.println("[WS] Initializing WebSocket client...");
}

void WebSocketMgr::loop() {
    _ws.loop();
}

bool WebSocketMgr::isConnected() {
    return _connected;
}

bool WebSocketMgr::sendMessage(const char* message) {
    if (!isConnected()) {
        Serial.println("[WS] Cannot send message - not connected");
        return false;
    }
    
    JsonDocument doc;
    doc["type"] = "ask";
    doc["text"] = message;

    char buffer[256];
    serializeJson(doc, buffer);

    _ws.sendTXT(buffer);
    Serial.print("[WS] Sent: ");
    Serial.println(buffer);
    return true;
}

bool WebSocketMgr::sendText(const char* text)
{
    if (!text || !isConnected() || !_sendQueue) {
        Serial.println("[WS] Cannot send message - not connected");
        return false;
    }

    WsMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = WsMsgType::TEXT;
    strncpy((char*)msg.data, text, WS_MSG_SIZE - 1);
    msg.len = strlen((const char*)msg.data);
    BaseType_t ok = xQueueSend(_sendQueue, &msg, pdMS_TO_TICKS(WS_QUEUE_SEND_WAIT_MS));

    return ok == pdTRUE;
}

bool WebSocketMgr::sendBinary(const uint8_t* data, size_t len)
{
    if (!data || len == 0 || len > WS_MSG_SIZE || !isConnected() || !_sendQueue) {
        return false;
    }

    WsMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = WsMsgType::BINARY;
    msg.len = len;
    memcpy(msg.data, data, len);
    BaseType_t ok = xQueueSend(_sendQueue, &msg, pdMS_TO_TICKS(WS_QUEUE_SEND_WAIT_MS));
    return ok == pdTRUE;
}

bool WebSocketMgr::sendHelloFromGabiAudio()
{
    if (!isConnected() || !_sendQueue) {
        return false;
    }

    size_t offset = 0;
    unsigned long startMs = millis();
    const unsigned long kTimeoutMs = 8000;

    while (offset < HELLO_FROM_GABI_PCM_LEN) {
        size_t chunk = HELLO_FROM_GABI_PCM_LEN - offset;
        if (chunk > AUDIO_FRAME_BYTES) {
            chunk = AUDIO_FRAME_BYTES;
        }

        if (sendBinary(HELLO_FROM_GABI_PCM + offset, chunk)) {
            offset += chunk;
            continue;
        }

        if (millis() - startMs > kTimeoutMs) {
            Serial.println("[WS][AUDIO] sendHelloFromGabiAudio timeout");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    unsigned long endStartMs = millis();
    while (!sendText("{\"type\":\"audio_end\"}")) {
        if (millis() - endStartMs > kTimeoutMs) {
            Serial.println("[WS][AUDIO] failed to queue audio_end");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    Serial.printf("[WS][AUDIO] hello_from_gabi queued bytes=%u\n", (unsigned)HELLO_FROM_GABI_PCM_LEN);
    return true;
}

void WebSocketMgr::startTask()
{
    Serial.println("[WS] startTask ");
    _sendQueue = xQueueCreate(WS_QUEUE_LEN, sizeof(WsMessage));
    if (_sendQueue == nullptr) {
        Serial.println("[WS] Failed to create send queue");
        return;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(
        task,
        "ws_task",
        WS_STACK_SIZE,
        this,
        1,
        NULL,
        0
    );
    if (ok != pdPASS) {
        Serial.println("[WS] Failed to create ws_task");
    }
}

// FreeRTOS task: runs WS library loop and drains the send queue every 10ms
void WebSocketMgr::task(void* pv)
{
    Serial.println("[WS] Task ");

    WebSocketMgr* self = static_cast<WebSocketMgr*>(pv);

    while (true)
    {
        self->_ws.loop();
        self->handleSendQueue();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Drain all pending messages from the send queue in a single pass
void WebSocketMgr::handleSendQueue()
{
    if (!_connected || !_sendQueue)
        return;

    WsMessage msg;
    while (xQueueReceive(_sendQueue, &msg, 0) == pdTRUE)
    {
        bool ok = false;
        if (msg.type == WsMsgType::TEXT) {
            ok = _ws.sendTXT((const char*)msg.data, msg.len);
        } else if (msg.type == WsMsgType::BINARY) {
            ok = _ws.sendBIN(msg.data, msg.len);
        }
        if (!ok) {
            Serial.println("[WS] Send FAIL");
        }
    }
}

// Static callback registered with WebSocketsClient — routes events to the singleton instance
void WebSocketMgr::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    if (g_instance == nullptr) {
        return;
    }

    String deviceInfo = "{\"type\":\"device_info\",\"name\":\"RobotAI_ESP32\"}";
    
    switch (type) {
        case WStype_DISCONNECTED:
            g_instance->_connected = false;
            g_spkBuf.clear();
            g_audioFramesRx = 0;
            g_audioFramesDropped = 0;
            g_audioSamplesDropped = 0;
            g_audioBytesRx = 0;
            g_lastAudioDropLogMs = 0;
            g_lastAudioRxLogMs = 0;
            Serial.println("[WS] Disconnected!");
            break;
            
        case WStype_CONNECTED:
            g_instance->_connected = true;
            g_spkBuf.clear();              // flush stale audio from previous session
            g_audioFramesRx = 0;
            g_audioFramesDropped = 0;
            g_audioSamplesDropped = 0;
            g_audioBytesRx = 0;
            g_lastAudioDropLogMs = 0;
            g_lastAudioRxLogMs = 0;
            Serial.println("[WS] Connected!");
            delay(1);
            g_instance->_ws.sendTXT(deviceInfo);  // announce device to server
            break;
            
        case WStype_TEXT:
            {
                g_instance->_connected = true;
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, payload, length);

                if (err) {
                    Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
                    break;
                }

                const char* answer   = nullptr;
                const char* userText = nullptr;

                if (doc["text"].is<const char*>())
                    answer = doc["text"].as<const char*>();
                if (doc["user_text"].is<const char*>())
                    userText = doc["user_text"].as<const char*>();

                if (answer && answer[0] != '\0') {
                    AiBridge::handleAnswer(answer, userText);
                } else {
                    const char* msgType = doc["type"].is<const char*>() ? doc["type"].as<const char*>() : "unknown";
                    Serial.printf("[WS] Ignore text frame without usable 'text' (type=%s)\n", msgType);
                }
            }
            break;
          
        case WStype_PING:
            Serial.println("[WS] Ping");
            
            break;
        case WStype_PONG:
            Serial.println("[WS] Pong");
            break;
            
        case WStype_BIN:
            {
                // Incoming binary = raw 16-bit PCM audio from server TTS
                if (length % 2 != 0) {
                    Serial.println("[WS] ERROR: Binary data length must be even");
                    break;
                }

                size_t samples = length / 2;
                int16_t* audioData = (int16_t*)payload;
                g_audioFramesRx++;
                g_audioBytesRx += (uint32_t)length;

                unsigned long now = millis();
                if (now - g_lastAudioRxLogMs >= 2000) {
                    g_lastAudioRxLogMs = now;
                    Serial.printf("[WS][AUDIO] rx frames=%lu bytes=%lu\n",
                                  (unsigned long)g_audioFramesRx,
                                  (unsigned long)g_audioBytesRx);
                }

                if (!g_spkBuf.push(audioData, samples)) {
                    g_audioFramesDropped++;
                    g_audioSamplesDropped += samples;

                    if (now - g_lastAudioDropLogMs >= 2000) {
                        g_lastAudioDropLogMs = now;
                        Serial.printf(
                            "[WS][AUDIO] drop frames=%lu/%lu samples=%lu\n",
                            (unsigned long)g_audioFramesDropped,
                            (unsigned long)g_audioFramesRx,
                            (unsigned long)g_audioSamplesDropped
                        );
                    }
                }
            }
            break;

        case WStype_ERROR:
            Serial.println("[WS] Error");
            break;
            
        default:
            Serial.printf("[WS] Unknown event type: %d\n", type);
            break;
    }
}

void WebSocketMgr::setEventCallback(std::function<void(String)> callback) {
    _messageCallback = callback;
}

// Public getter methods
const char* WebSocketMgr::getServer() {
    return WS_SERVER;
}

const int WebSocketMgr::getPort() {
    return WS_PORT;
}

const char* WebSocketMgr::getPath() {
    return WS_PATH;
}
