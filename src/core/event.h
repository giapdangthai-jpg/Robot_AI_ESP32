#pragma once
#include <Arduino.h>

enum class EventType {
    NONE,
    CMD_FORWARD,
    CMD_STOP,
    AUDIO_FRAME,
};

struct Event {
    EventType type;
    int value;
};
