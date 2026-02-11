#include "motor.h"
#include "../core/event_bus.h"
#include "../core/event.h"
#include <Arduino.h>

static void motorTask(void* pv) {

    Event e;

    while (true) {
        if (EventBus::receive(e)) {

            switch (e.type) {

                case EventType::CMD_FORWARD:
                    Motor::forward();
                    break;

                case EventType::CMD_STOP:
                    Motor::stop();
                    break;

                default:
                    break;
            }
        }
    }
}

void startMotorTask() {
    xTaskCreatePinnedToCore(
        motorTask,
        "motorTask",
        4096,
        NULL,
        2,
        NULL,
        1
    );
}
