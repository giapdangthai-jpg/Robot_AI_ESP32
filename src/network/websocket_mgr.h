#pragma once
#include <string>
#include <functional>
#include <WebSocketsClient.h>

class WebSocketMgr {
public:
    void init();
    void loop();
    bool isConnected();
    bool sendMessage(const char* message);
    bool sendText(const char* text);
    bool sendHelloFromGabiAudio();
    bool sendBinary(const uint8_t* data, size_t len);
    void setEventCallback(std::function<void(String)> callback);
    void setConnectCallback(std::function<void()> callback);
    void startTask();
    const char* getServer();
    const int getPort();
    const char* getPath();
private:
    WebSocketsClient _ws;
    QueueHandle_t _sendQueue = nullptr;
    bool _connected = false;
    std::function<void(String)> _messageCallback = nullptr;
    std::function<void()> _connectCallback = nullptr;
    static unsigned long _lastConnectAttempt;
    static const unsigned long RECONNECT_INTERVAL = 3000;
    // Host and port are loaded at runtime from ConfigStore (NVS)
    static constexpr const char* WS_PATH = "/ws/public";

    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static void task(void* pv);
    void handleSendQueue();
};
