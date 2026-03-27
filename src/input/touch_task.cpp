#include "touch_task.h"
#include "../config/pinmap.h"
#include "../core/event_bus.h"
#include "../core/event.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/touch_sensor.h>

// GPIO4 = TOUCH_PAD_NUM4 trên ESP32-S3-DevKitC-1
#define TOUCH_PAD       TOUCH_PAD_NUM4
#define TOUCH_THRESHOLD 29000   // idle ~27000, touched ~31000
#define DEBOUNCE_MS     400

static void touchTask(void*) {
    int8_t touchCount = 0;
    bool wasAbove = false;

    for (;;) {
        uint32_t val = 0;
        touch_pad_read_raw_data(TOUCH_PAD, &val);
        //Serial.printf("[TOUCH] val=%lu\n", (unsigned long)val);
        bool above = (val > TOUCH_THRESHOLD);

        if (above && !wasAbove) {
            switch (touchCount) {
                case 0:
                    Serial.println("[TOUCH] Lie");
                    EventBus::publish({EventType::CMD_LIE, 0});
                    touchCount = 1;
                    break;
                case 1:
                    Serial.println("[TOUCH] Stand");
                    EventBus::publish({EventType::CMD_STAND, 0});
                    touchCount = 0;
                    break;
            }
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }
        wasAbove = above;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void startTouchTask() {
    touch_pad_init();
    touch_pad_set_voltage(
        TOUCH_HVOLT_2V7,
        TOUCH_LVOLT_0V5,
        TOUCH_HVOLT_ATTEN_1V
    );
    touch_pad_config(TOUCH_PAD);
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreatePinnedToCore(touchTask, "touch", 4096, nullptr, 1, nullptr, 1);
}
