#include "websocket_mgr.h"
#include <WebSocketsClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../ai/ai_bridge.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
}

#define WS_STACK_SIZE 8192
#define WS_QUEUE_LEN  10
#define WS_MSG_SIZE   256
typedef struct
{
    char data[WS_MSG_SIZE];
} WsMessage;

static WebSocketMgr* g_instance = nullptr;
// static WebSocketsClient ws;

// Static member definitions
//std::function<void(String)> WebSocketMgr::_messageCallback = nullptr;
//unsigned long WebSocketMgr::_lastConnectAttempt = 0;

void WebSocketMgr::init() {
    g_instance = this;
    _sendQueue = xQueueCreate(WS_QUEUE_LEN, sizeof(WsMessage));
    
    _ws.begin(WS_SERVER, WS_PORT, WS_PATH);
    _ws.onEvent(webSocketEvent);
    _ws.setReconnectInterval(RECONNECT_INTERVAL);
    _ws.enableHeartbeat(30000, 5000, 2);
    startTask();
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
    Serial.println("[WS] Sendtext ");

    if (!isConnected() || !_sendQueue) {
        Serial.println("[WS] Cannot send message - not connected");
        return false;
    }

    WsMessage msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.data, text, WS_MSG_SIZE - 1);
    BaseType_t ok = xQueueSend(_sendQueue, &msg, 0);

    return ok == pdTRUE;
    // Serial.println(result ? "[WS] Sent Text Sucess: " : "[WS] Sent Text failed: ");
    // return result;
}

void WebSocketMgr::startTask()
{
    Serial.println("[WS] startTask ");

    xTaskCreatePinnedToCore(
        task,
        "ws_task",
        WS_STACK_SIZE,
        this,
        1,
        NULL,
        0
    );
}

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
void WebSocketMgr::handleSendQueue()
{
    if (!_connected)
        return;

    char buffer[WS_MSG_SIZE];
    WsMessage msg;
    while (xQueueReceive(_sendQueue, &msg, 0) == pdTRUE)
    {
        Serial.println("[WS] handleSendQueue - while");

        bool ok = _ws.sendTXT(msg.data);
        Serial.println(ok ? "[WS] Send OK" : "[WS] Send FAIL");
    }
}
void WebSocketMgr::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    String deviceInfo = "{\"type\":\"device_info\",\"name\":\"RobotAI_ESP32\"}";
    
    switch (type) {
        case WStype_DISCONNECTED:
            g_instance->_connected = false;
            Serial.println("[WS] Disconnected!");
            break;
            
        case WStype_CONNECTED:
            g_instance->_connected = true;
            Serial.println("[WS] Connected!");
            delay(1);
            if (g_instance != nullptr)
                {
                    g_instance->_ws.sendTXT(deviceInfo);
                }
            break;
            
        case WStype_TEXT:
            {
                g_instance->_connected = true;
                JsonDocument doc;   // sửa luôn deprecated
                DeserializationError err = deserializeJson(doc, payload);

                if (!err)
                {
                    const char* answer = doc["text"];
                    AiBridge::handleAnswer(answer);
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
            Serial.printf("[WS] Binary received: %u bytes\n", length);
            Serial.printf("[WS] BIN first byte: %d\n", payload[0]);
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
