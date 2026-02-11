#include <WebSocketsClient.h>
#include "../utils/audio_buffer.h"
#include "../config/pinmap.h"

static WebSocketsClient ws;
static bool connected = false;

static void onEvent(WStype_t type, uint8_t * payload, size_t length) {

    if (type == WStype_CONNECTED) {
        connected = true;
    }

    if (type == WStype_DISCONNECTED) {
        connected = false;
    }
}

static void wsTask(void* pv) {

    int16_t frame[AUDIO_FRAME_SAMPLES];

    ws.begin("192.168.1.100", 8080, "/audio");
    ws.onEvent(onEvent);
    ws.setReconnectInterval(2000);

    while (true) {

        ws.loop();

        if (connected) {

            if (AudioBuffer::pop(frame, AUDIO_FRAME_SAMPLES)) {

                ws.sendBIN(
                    (uint8_t*)frame,
                    AUDIO_FRAME_BYTES
                );
            }
        }

        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}
