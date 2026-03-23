#pragma once
#include <string>
#include <functional>
#include <WebSocketsClient.h>

class WebSocketMgr {
public:
    void init();
    bool isConnected();
    bool sendMessage(const char* message);
    bool sendText(const char* text);
    bool sendBinary(const uint8_t* data, size_t len);
    void setConnectCallback(std::function<void()> callback);
    void startTask();
private:
    WebSocketsClient _ws;
    QueueHandle_t _sendQueue = nullptr;
    bool _connected = false;
    std::function<void()> _connectCallback = nullptr;
    static const unsigned long RECONNECT_INTERVAL = 3000;
    static constexpr const char* WS_PATH = "/ws/public";

    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static void task(void* pv);
    void handleSendQueue();
};
