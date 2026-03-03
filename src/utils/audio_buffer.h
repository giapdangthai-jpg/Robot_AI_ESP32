#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>

class AudioBuffer {
public:
    void init();
    void clear();
    bool push(const int16_t* data, size_t samples);
    bool pop(int16_t* data, size_t samples);

private:
    static constexpr size_t kCapacity = 8192;
    int16_t _buf[kCapacity];
    size_t _writeIdx = 0;
    size_t _readIdx  = 0;
    size_t _count    = 0;
    SemaphoreHandle_t _mutex = nullptr;
};

extern AudioBuffer g_micBuf;
extern AudioBuffer g_spkBuf;
