#pragma once
#include <stdint.h>
#include <stddef.h>

// In-place audio preprocessing: HPF → AGC
//
// HPF  — first-order DC-removal / high-pass (fc ≈ 80 Hz @ 16 kHz).
//        Removes DC offset, 50/60 Hz mains hum, and robot-body vibration
//        before the signal reaches VAD or the server.
//
// AGC  — peak-follower automatic gain control.
//        Fast attack (prevents clipping), slow release (avoids pumping).
//        Replaces the fixed bit-shift gain: output level is normalised
//        regardless of microphone placement or speaker distance.
class AudioPreproc {
public:
    // Apply HPF then AGC to `frame` in-place.
    static void process(int16_t* frame, size_t count);

    // Reset filter state (call on connection loss or speaker-active edge).
    static void reset();
};
