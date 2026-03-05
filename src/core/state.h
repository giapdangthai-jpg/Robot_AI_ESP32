#pragma once

// Top-level robot operating states
enum class RobotState {
    IDLE,    // no active task
    WALK,    // motor movement in progress
    LISTEN,  // mic active, waiting for speech
    SPEAK    // speaker playing TTS audio
};

// Global state machine — simple get/set, not thread-safe (extend with mutex if needed)
class StateMachine {
public:
    static void set(RobotState s);
    static RobotState get();
};
