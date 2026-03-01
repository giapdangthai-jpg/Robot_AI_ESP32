#include "websocket.h"
#include "../core/event_bus.h"
#include "../core/event.h"
#include <WebSocketsClient.h>

static WebSocketsClient ws;

static void onMessage(WStype_t type, uint8_t* payload, size_t length) {

    if (type == WStype_TEXT) {

        if (strcmp((char*)payload, "forward") == 0) {
            EventBus::publish({EventType::CMD_FORWARD, 0});
        }

        if (strcmp((char*)payload, "stop") == 0) {
            EventBus::publish({EventType::CMD_STOP, 0});
        }
    }
}

static void wsTask(void* pv) {

    ws.begin("192.168.100.170", 8000, "/ws/public");
    ws.onEvent(onMessage);

    while (true) {
        ws.loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void startWebSocketTask() {
    xTaskCreatePinnedToCore(
        wsTask,
        "wsTask",
        8192,
        NULL,
        1,
        NULL,
        0
    );
}
