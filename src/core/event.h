#pragma once
#include <Arduino.h>

// All command types dispatched through EventBus to motor/audio tasks
enum class EventType {
    NONE,
    // Movement
    CMD_RUN_FORWARD,
    CMD_RUN_BACKWARD,
    CMD_WALK_FORWARD,
    CMD_WALK_BACKWARD,
    CMD_STOP,
    CMD_TURN_LEFT,
    CMD_TURN_RIGHT,
    CMD_SPEED_UP,
    CMD_SLOW_DOWN,
    CMD_DANCE,
    // Posture
    CMD_SIT,
    CMD_STAND,
    CMD_LIE,
    // Audio
    CMD_VOLUME_UP,
    CMD_VOLUME_DOWN,
};

// Event passed through the FreeRTOS queue; value is command-specific payload
struct Event {
    EventType type;
    int value;
};
