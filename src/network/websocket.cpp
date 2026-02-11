#include "websocket.h"
#include <WebSocketsClient.h>

static WebSocketsClient ws;

void WebSocketClient::init() {
    ws.begin("192.168.1.100", 8080, "/");
}

void WebSocketClient::loop() {
    ws.loop();
}
