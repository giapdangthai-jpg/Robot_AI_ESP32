#include "state.h"

static RobotState currentState = RobotState::IDLE;

void StateMachine::set(RobotState s) {
    currentState = s;
}

RobotState StateMachine::get() {
    return currentState;
}
