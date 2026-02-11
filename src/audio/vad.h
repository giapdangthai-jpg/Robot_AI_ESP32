#pragma once
#include <stdint.h>

class VAD {
public:
    static bool isSpeech(int16_t* samples, size_t count);
};
