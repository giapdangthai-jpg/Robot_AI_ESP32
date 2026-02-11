#include "event_bus.h"

QueueHandle_t EventBus::queue = nullptr;

void EventBus::init() {
    queue = xQueueCreate(20, sizeof(Event));
}

bool EventBus::publish(const Event& e) {
    if (!queue) return false;
    return xQueueSend(queue, &e, 0) == pdTRUE;
}

bool EventBus::receive(Event& e) {
    if (!queue) return false;
    return xQueueReceive(queue, &e, portMAX_DELAY) == pdTRUE;
}
