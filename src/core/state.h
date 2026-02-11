#pragma once

enum class RobotState {
    IDLE,
    WALK,
    LISTEN,
    SPEAK
};

class StateMachine {
public:
    static void set(RobotState s);
    static RobotState get();
};
