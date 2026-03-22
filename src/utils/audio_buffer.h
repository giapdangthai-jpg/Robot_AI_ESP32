#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>

class AudioBuffer {
public:
    bool init(size_t capacitySamples = 8192, bool preferPsram = false);
    void clear();
    bool   push(const int16_t* data, size_t samples);
    bool   pop(int16_t* data, size_t samples);
    size_t size() const { return _count; }

private:
    int16_t* _buf = nullptr;
    size_t _capacity = 0;
    bool _usingPsram = false;
    size_t _writeIdx = 0;
    size_t _readIdx  = 0;
    size_t _count    = 0;
    SemaphoreHandle_t _mutex = nullptr;
};

extern AudioBuffer g_micBuf;
extern AudioBuffer g_spkBuf;

// Set true while speaker is playing; mic_upload reads this to mute uplink
extern volatile bool g_speakerActive;
