#pragma once
#include <stdint.h>
#include <stddef.h>

// In-place audio preprocessing: HPF → Pre-emphasis → AGC
//
// HPF  — 2nd-order Butterworth high-pass (fc = 80 Hz @ 16 kHz, Q = 0.707).
//        Removes DC offset, 50/60 Hz hum, and robot-body vibration.
//        -40 dB/decade roll-off vs 1st-order.
//
// PE   — pre-emphasis FIR: y[n] = x[n] - 0.97·x[n-1].
//        Boosts high-frequency consonants (s, f, sh, th) to compensate for
//        the INMP441 capsule roll-off and improve Whisper STT accuracy.
//
// AGC  — peak-follower automatic gain control.
//        Fast attack (prevents clipping), slow release (avoids pumping).
//        Normalises output level regardless of mic distance.
//
// Cross-core safety:
//   process() runs on Core 1 (mic_task).
//   requestReset() is safe to call from Core 0 (mic_upload_task):
//   it sets a volatile flag; process() drains it at the next frame boundary.
//   On Xtensa LX7 a single-byte write is atomic — volatile is sufficient.
class AudioPreproc {
public:
    // Apply HPF → pre-emphasis → AGC in-place. Called from Core 1.
    static void process(int16_t* frame, size_t count);

    // Request a filter-state reset from Core 0. Thread-safe.
    // The reset executes at the start of the next process() call on Core 1.
    static void requestReset();
};
