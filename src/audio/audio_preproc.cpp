#include "audio_preproc.h"

namespace {

// ── High-Pass Filter ─────────────────────────────────────────────────────────
// DC-removal HPF:  y[n] = x[n] - x[n-1] + R·y[n-1]
// R = 1 - 2π·fc/fs  →  fc ≈ 80 Hz @ 16 kHz  →  R ≈ 0.9686
// Integer approx: R = 31/32 = 0.96875  →  fc ≈ 79 Hz
// Attenuates DC, motor vibration, and chassis resonance.
static int32_t s_hpf_x = 0;  // x[n-1]
static int32_t s_hpf_y = 0;  // y[n-1]

static inline int16_t hpf_sample(int16_t xn) {
    int32_t yn = (int32_t)xn - s_hpf_x + (s_hpf_y * 31) / 32;
    s_hpf_x = xn;
    s_hpf_y = yn;
    return yn > 32767 ? 32767 : (yn < -32768 ? -32768 : (int16_t)yn);
}

// ── Automatic Gain Control ────────────────────────────────────────────────────
// Peak-follower: measures peak |sample| per frame, nudges gain so output peak
// approaches AGC_TARGET_PEAK.
//   Attack  — fast (1 frame): prevents clipping on loud/close speech.
//   Release — slow (~1 % per frame): avoids audible pumping on silence.
// Gain is stored in Q8 fixed-point (256 = 1.0×).
static constexpr int32_t AGC_TARGET_PEAK  = 6000;    // ~18 % FS — good for STT
static constexpr int32_t AGC_MIN_GAIN_Q8  = 1 * 256; // 1× floor — no attenuation
static constexpr int32_t AGC_MAX_GAIN_Q8  = 24 * 256;// 24× ceiling
static constexpr int32_t AGC_RELEASE_NUM  = 101;
static constexpr int32_t AGC_RELEASE_DEN  = 100;     // +1 % per frame

static int32_t s_agc_gain_q8 = 8 * 256;  // start at 8× (conservative; adapts quickly)

} // namespace

// ── Public API ────────────────────────────────────────────────────────────────

void AudioPreproc::process(int16_t* frame, size_t count) {
    if (!frame || count == 0) return;

    // Pass 1 — HPF (removes DC offset and low-frequency rumble)
    for (size_t i = 0; i < count; ++i) {
        frame[i] = hpf_sample(frame[i]);
    }

    // Pass 2 — measure peak for AGC
    int32_t peak = 0;
    for (size_t i = 0; i < count; ++i) {
        int32_t a = frame[i] < 0 ? -(int32_t)frame[i] : (int32_t)frame[i];
        if (a > peak) peak = a;
    }

    // Pass 3 — update gain
    if (peak > 0) {
        int32_t desired_q8 = (AGC_TARGET_PEAK << 8) / peak;
        if (desired_q8 < AGC_MIN_GAIN_Q8) desired_q8 = AGC_MIN_GAIN_Q8;
        if (desired_q8 > AGC_MAX_GAIN_Q8) desired_q8 = AGC_MAX_GAIN_Q8;

        if (desired_q8 < s_agc_gain_q8) {
            s_agc_gain_q8 = desired_q8;                              // fast attack
        } else {
            s_agc_gain_q8 = s_agc_gain_q8 * AGC_RELEASE_NUM / AGC_RELEASE_DEN;
            if (s_agc_gain_q8 > desired_q8) s_agc_gain_q8 = desired_q8; // slow release
        }
    }

    // Pass 4 — apply gain with saturation
    for (size_t i = 0; i < count; ++i) {
        int32_t s = ((int32_t)frame[i] * s_agc_gain_q8) >> 8;
        frame[i] = s > 32767 ? 32767 : (s < -32768 ? -32768 : (int16_t)s);
    }
}

void AudioPreproc::reset() {
    s_hpf_x      = 0;
    s_hpf_y      = 0;
    s_agc_gain_q8 = 8 * 256;
}
