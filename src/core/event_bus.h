#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "event.h"

class EventBus {
public:
    static void init();
    static bool publish(const Event& e);
    static bool receive(Event& e);

private:
    static QueueHandle_t queue;
};
