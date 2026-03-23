#pragma once
#include <stddef.h>
#include <stdint.h>

class VAD {
public:
    // Call every IDLE frame to adapt threshold to ambient noise level
    static void updateNoiseFloor(const int16_t* samples, size_t count);

    // Returns true if frame energy is significantly above the estimated noise floor
    static bool isSpeech(const int16_t* samples, size_t count);
};
