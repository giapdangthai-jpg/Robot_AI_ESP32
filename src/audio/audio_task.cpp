#include "../core/event_bus.h"
#include "../core/event.h"
#include <Arduino.h>

static void audioTask(void* pv) {

    while (true) {

        // giả lập audio frame
        EventBus::publish({EventType::AUDIO_FRAME, 1});

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void startAudioTask() {
    xTaskCreatePinnedToCore(
        audioTask,
        "audioTask",
        8192,
        NULL,
        1,
        NULL,
        0
    );
}
