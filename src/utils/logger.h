#pragma once
#include <Arduino.h>

class Logger {
public:
    static void info(const char* msg) {
        Serial.println(msg);
    }
};
