#include "motor.h"
#include "../config/pinmap.h"
#include <Arduino.h>

void Motor::init() {
    pinMode(M1_IN1, OUTPUT);
    pinMode(M1_IN2, OUTPUT);
    pinMode(M2_IN1, OUTPUT);
    pinMode(M2_IN2, OUTPUT);
}

void Motor::forward() {
    digitalWrite(M1_IN1, HIGH);
    digitalWrite(M1_IN2, LOW);
    digitalWrite(M2_IN1, HIGH);
    digitalWrite(M2_IN2, LOW);
}

void Motor::stop() {
    digitalWrite(M1_IN1, LOW);
    digitalWrite(M1_IN2, LOW);
    digitalWrite(M2_IN1, LOW);
    digitalWrite(M2_IN2, LOW);
}
