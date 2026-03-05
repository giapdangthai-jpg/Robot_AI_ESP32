#include "vad.h"

namespace {
// RMS energy threshold (amplitude units); tune if VAD triggers on noise or misses speech
static constexpr int16_t kVadThreshold = 500;
}

// Energy-based VAD: returns true if mean square energy of the frame exceeds threshold^2
// Avoids float: computes mean(s^2) in integer and compares to threshold^2
bool VAD::isSpeech(const int16_t* samples, size_t count) {
    if (!samples || count == 0) {
        return false;
    }

    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        const int32_t s = samples[i];
        sum += s * s;
    }

    const int64_t threshold2 = (int64_t)kVadThreshold * kVadThreshold;
    return (sum / (int64_t)count) > threshold2;
}
