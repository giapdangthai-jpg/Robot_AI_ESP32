#pragma once
#include <stddef.h>
#include <stdint.h>

class SpeakerI2S {
public:
    static void init();
    static bool write(const int16_t* samples, size_t bytes);
};
