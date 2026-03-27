#include "vad.h"

// ── Client-side VAD — bandwidth gate only ────────────────────────────────────
//
// Role in the pipeline:
//   ESP32: HPF → AGC → VAD gate → stream → Server Silero VAD → STT
//
// This VAD does NOT decide "is this speech?".
// Its sole job is to gate out ABSOLUTE SILENCE so the ESP32 does not
// stream dead air, reducing server load when scaling to multiple robots.
//
// Design intent:
//   • Very high sensitivity (low false-negative rate) — better to stream
//     a little noise than to miss the start of a command.
//   • Server Silero VAD makes the real speech/non-speech decision.
//   • Thresholds are intentionally permissive: kSpeechRatioSq = 2 (1.41×),
//     kMinThresholdSq tuned for post-HPF+AGC signal levels.

namespace {

// ── Adaptive noise floor ─────────────────────────────────────────────────────
// EMA tracks ambient energy during silence.
// alpha = 10/100 = 0.10 — adapts in ~30 IDLE frames (0.6 s).
// Faster than before so the gate recovers quickly after speech.
static int64_t s_noiseFloorSq = 10000LL;  // initial estimate; overwritten during streaming

static constexpr int64_t kAlphaNumer = 10;
static constexpr int64_t kAlphaDenom = 100;

// ── Gate thresholds ──────────────────────────────────────────────────────────
// kSpeechRatioSq = 2  →  frame must be only 1.41× above noise floor.
// Very permissive: Silero VAD on the server rejects the false positives.
static constexpr int64_t kSpeechRatioSq = 2;

// Hard floor: frames below this are absolute silence and never streamed.
// ~100 RMS after HPF+AGC — well below normal speech but above hardware hiss.
static constexpr int64_t kMinThresholdSq = 10000LL;

// ── Helper ───────────────────────────────────────────────────────────────────
static int64_t computeMeanSq(const int16_t* samples, size_t count) {
    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        const int32_t s = samples[i];
        sum += s * s;
    }
    return sum / (int64_t)count;
}

} // namespace

// ── Public API ────────────────────────────────────────────────────────────────

void VAD::updateNoiseFloor(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return;
    const int64_t meanSq = computeMeanSq(samples, count);
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
