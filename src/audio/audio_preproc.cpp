#include "audio_preproc.h"

// Cross-core reset flag: set by requestReset() on Core 0,
// consumed at the top of process() on Core 1.
// Xtensa LX7: single-byte read/write is atomic; volatile prevents caching.
static volatile bool s_resetPending = false;

namespace {

// ── High-Pass Filter (Biquad, 2nd-order Butterworth) ─────────────────────────
// Design: fc = 80 Hz @ fs = 16 kHz, Q = 1/√2 (maximally flat / Butterworth)
// -40 dB/decade roll-off vs -20 dB for 1st-order — better motor vibration rejection.
//
// Bilinear-transform coefficients (Q14 fixed-point, scale = 2^14 = 16384):
//   ω0 = 2π·80/16000, α = sin(ω0)/(2Q)
//   B0 =  16026  (≈ +0.97803 · 2^14)
//   B1 = -32053  (≈ -1.95606 · 2^14)
//   B2 =  16026
//   A1 =  32051  (positive: this is −a1_norm, since a1_norm ≈ −1.95596)
//   A2 = -15673  (negative: this is −a2_norm, since a2_norm ≈ +0.95653)
//
// Difference equation (Direct Form I):
//   y[n] = (B0·x[n] + B1·x1 + B2·x2 + A1·y1 + A2·y2) >> 14
//
// All intermediate arithmetic uses int64 to prevent overflow.

static constexpr int32_t HPF_B0 =  16026;
static constexpr int32_t HPF_B1 = -32053;
static constexpr int32_t HPF_B2 =  16026;
static constexpr int32_t HPF_A1 =  32051;  // = -a1_norm * 2^14
static constexpr int32_t HPF_A2 = -15673;  // = -a2_norm * 2^14

static int32_t s_hpf_x1 = 0, s_hpf_x2 = 0;  // x[n-1], x[n-2]
static int32_t s_hpf_y1 = 0, s_hpf_y2 = 0;  // y[n-1], y[n-2]

static inline int16_t hpf_sample(int16_t xn) {
    int64_t acc = (int64_t)HPF_B0 * xn
                + (int64_t)HPF_B1 * s_hpf_x1
                + (int64_t)HPF_B2 * s_hpf_x2
                + (int64_t)HPF_A1 * s_hpf_y1
                + (int64_t)HPF_A2 * s_hpf_y2;
    int32_t yn = (int32_t)(acc >> 14);
    if (yn >  32767) yn =  32767;
    if (yn < -32768) yn = -32768;
    s_hpf_x2 = s_hpf_x1;  s_hpf_x1 = xn;
    s_hpf_y2 = s_hpf_y1;  s_hpf_y1 = yn;
    return (int16_t)yn;
}

// ── Pre-emphasis Filter ───────────────────────────────────────────────────────
// Boosts high frequencies (consonants: s, f, sh, th) to compensate for the
// natural roll-off of the INMP441 capsule and improve Whisper STT accuracy.
//
// First-order FIR: y[n] = x[n] - α·x[n-1]   α = 0.97 → fc_shelf ≈ 300 Hz @ 16 kHz
// Q8 integer approx: α_q8 = 248  (= round(0.97 × 256))
// Applied AFTER HPF (DC removed) and BEFORE AGC (level normalised on
// the pre-emphasised spectrum).

static constexpr int32_t PE_ALPHA_Q8 = 248;  // 0.97 × 256
static int32_t s_pe_x1 = 0;                  // x[n-1]

static inline int16_t pe_sample(int16_t xn) {
    int32_t yn = (int32_t)xn - ((PE_ALPHA_Q8 * s_pe_x1) >> 8);
    s_pe_x1 = xn;
    if (yn >  32767) yn =  32767;
    if (yn < -32768) yn = -32768;
    return (int16_t)yn;
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

static void doReset() {
    s_hpf_x1 = s_hpf_x2 = 0;
    s_hpf_y1 = s_hpf_y2 = 0;
    s_pe_x1        = 0;
    s_agc_gain_q8  = 8 * 256;
}

void AudioPreproc::requestReset() {
    s_resetPending = true;   // Core 0 → Core 1 signal; single-byte write is atomic on LX7
}

void AudioPreproc::process(int16_t* frame, size_t count) {
    if (!frame || count == 0) return;

    // Drain pending reset request from Core 0 before touching filter state
    if (s_resetPending) {
        doReset();
        s_resetPending = false;
    }

    // Pass 1 — HPF (removes DC offset and low-frequency rumble below 80 Hz)
    for (size_t i = 0; i < count; ++i) {
        frame[i] = hpf_sample(frame[i]);
    }

    // Pass 2 — pre-emphasis (boost high-freq consonants for STT)
    for (size_t i = 0; i < count; ++i) {
        frame[i] = pe_sample(frame[i]);
    }

    // Pass 3 — measure peak for AGC
    int32_t peak = 0;
    for (size_t i = 0; i < count; ++i) {
        int32_t a = frame[i] < 0 ? -(int32_t)frame[i] : (int32_t)frame[i];
        if (a > peak) peak = a;
    }

    // Pass 4 — update gain
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

    // Pass 5 — apply gain with saturation
    for (size_t i = 0; i < count; ++i) {
        int32_t s = ((int32_t)frame[i] * s_agc_gain_q8) >> 8;
        frame[i] = s > 32767 ? 32767 : (s < -32768 ? -32768 : (int16_t)s);
    }
}

