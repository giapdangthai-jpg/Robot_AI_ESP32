#include "vad.h"

namespace {

// ── Adaptive noise floor ────────────────────────────────────────────────────
// Tracks mean-square energy during IDLE via exponential moving average.
// All arithmetic is integer (int64_t) — no float, no sqrt needed.

// Initial noise floor estimate (≈ 300 RMS → 300² = 90 000)
static int64_t s_noiseFloorSq = 90000LL;

// EMA alpha = 5/100 = 0.05 — slow adaptation so a brief loud event doesn't
// corrupt the baseline. Takes ~60 IDLE frames (~1.2 s) to fully adapt.
static constexpr int64_t kAlphaNumer = 5;
static constexpr int64_t kAlphaDenom = 100;

// Speech must be at least kSpeechRatio × louder than noise floor.
// Threshold is compared in squared domain: ratio² = 9 means 3× RMS.
static constexpr int64_t kSpeechRatioSq = 9;   // 3²

// Hard minimum so VAD never fires on absolute silence / hardware hiss
// (400 RMS → 400² = 160 000)
static constexpr int64_t kMinThresholdSq = 160000LL;

// ── Helper ──────────────────────────────────────────────────────────────────
static int64_t computeMeanSq(const int16_t* samples, size_t count) {
    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        const int32_t s = samples[i];
        sum += s * s;
    }
    return sum / (int64_t)count;
}

} // namespace

// ── Public API ───────────────────────────────────────────────────────────────

void VAD::updateNoiseFloor(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return;
    const int64_t meanSq = computeMeanSq(samples, count);
    // EMA: floor = alpha * current + (1 - alpha) * floor
    s_noiseFloorSq = (kAlphaNumer * meanSq +
                      (kAlphaDenom - kAlphaNumer) * s_noiseFloorSq) / kAlphaDenom;
}

bool VAD::isSpeech(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return false;
    const int64_t meanSq    = computeMeanSq(samples, count);
    const int64_t threshold = (s_noiseFloorSq * kSpeechRatioSq > kMinThresholdSq)
                                  ? s_noiseFloorSq * kSpeechRatioSq
                                  : kMinThresholdSq;
    return meanSq > threshold;
}
