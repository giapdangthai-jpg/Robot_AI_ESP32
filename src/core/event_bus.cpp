#include "event_bus.h"

QueueHandle_t EventBus::queue = nullptr;

// Create the FreeRTOS queue (capacity: 20 events)
void EventBus::init() {
    queue = xQueueCreate(20, sizeof(Event));
}

// Non-blocking publish — returns false if queue is full
bool EventBus::publish(const Event& e) {
    if (!queue) return false;
    return xQueueSend(queue, &e, 0) == pdTRUE;
}

// Blocking receive — caller task blocks until an event arrives
bool EventBus::receive(Event& e) {
    if (!queue) return false;
    return xQueueReceive(queue, &e, portMAX_DELAY) == pdTRUE;
}
