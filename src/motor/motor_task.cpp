#include "motor.h"
#include "../core/event_bus.h"
#include "../core/event.h"
#include <Arduino.h>

static void motorTask(void* pv) {

    Event e;

    while (true) {
        if (EventBus::receive(e)) {

            switch (e.type) {

                case EventType::CMD_RUN_FORWARD:
                    Motor::runForward();
                    break;
                case EventType::CMD_RUN_BACKWARD:
                    Motor::runBackward();
                    break;
                case EventType::CMD_TURN_LEFT:
                    Motor::turnLeft();
                    break;
                case EventType::CMD_TURN_RIGHT:
                    Motor::turnRight();
                    break;
                case EventType::CMD_WALK_FORWARD:
                    Motor::walkForward();
                    break;
                case EventType::CMD_WALK_BACKWARD:
                    Motor::walkBackward();
                    break;
                case EventType::CMD_SIT:
                    Motor::sit();
                    break;
                case EventType::CMD_STAND:
                    Motor::stand();
                    break;
                case EventType::CMD_LIE:
                    Motor::lie();
                    break;
                case EventType::CMD_DANCE:
                    Motor::dance();
                    break;
                case EventType::CMD_SPEED_UP:
                    Motor::speedUp();
                    break;
                case EventType::CMD_SLOW_DOWN:
                    Motor::slowDown();
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
