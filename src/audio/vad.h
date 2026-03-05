#pragma once
#include <stddef.h>
#include <stdint.h>

class VAD {
public:
    static bool isSpeech(const int16_t* samples, size_t count);
};
