#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class AudioBuffer {
public:
    static void init();
    static bool push(int16_t* data, size_t samples);
    static bool pop(int16_t* data, size_t samples);

private:
    static int16_t buffer[8192];
    static size_t writeIndex;
    static size_t readIndex;
    static SemaphoreHandle_t mutex;
};
