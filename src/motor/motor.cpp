#include "motor.h"
#include "../config/pinmap.h"
#include <Arduino.h>

// ── M3 back-leg ADC position targets (12-bit: 0–4095) ───────
// Tune by reading Serial "[MOTOR] M3 pos=..." after physical assembly
static constexpr int      M3_POS_LIE     = 500;   // lie down
static constexpr int      M3_POS_SIT     = 2000;  // sit
static constexpr int      M3_POS_STAND   = 3500;  // stand upright
static constexpr int      M3_POS_TOL     = 200;   // acceptable ADC error
static constexpr uint32_t M3_TIMEOUT_MS  = 3000;  // M3 control timeout (ms)
static constexpr uint32_t WALK_STEP_MS   = 150;   // duration per half-step (ms), tune after assembly

// ── Current speed/direction state ────────────────────────────
// Retained by speedUp/slowDown to re-apply when changing speed
static uint8_t g_speed = 200;  // PWM duty cycle (0–255)
static int8_t  g_dirM1 = 0;    // M1 direction: +1 fwd, -1 bwd, 0 stop
static int8_t  g_dirM2 = 0;    // M2 direction: +1 fwd, -1 bwd, 0 stop

// ── Internal helpers ─────────────────────────────────────────

// Apply speed and direction to M1 (left leg)
// DRV8833: PWM on IN1 → forward; PWM on IN2 → backward; both LOW → coast
static void applyM1(int8_t dir) {
    g_dirM1 = dir;
    if (dir > 0) {
        analogWrite(M1_IN1, g_speed);
        digitalWrite(M1_IN2, LOW); }
    else if (dir < 0) {
        digitalWrite(M1_IN1, LOW);
        analogWrite(M1_IN2, g_speed); }
    else {
        digitalWrite(M1_IN1, LOW);
        digitalWrite(M1_IN2, LOW); }
}

// Apply speed and direction to M2 (right leg)
static void applyM2(int8_t dir) {
    g_dirM2 = dir;
    if (dir > 0) {
        analogWrite(M2_IN1, g_speed);
        digitalWrite(M2_IN2, LOW); }
    else if (dir < 0) {
        digitalWrite(M2_IN1, LOW);
        analogWrite(M2_IN2, g_speed); }
    else {
        digitalWrite(M2_IN1, LOW);
        digitalWrite(M2_IN2, LOW); }
}

// Apply direction to M3 (robot back – posture control)
// M3 uses full ON/OFF only; position is controlled via ADC feedback
static void applyM3(int8_t dir) {
    if (dir > 0) {
        digitalWrite(M3_IN1, HIGH);
        digitalWrite(M3_IN2, LOW); }
    else if (dir < 0) {
        digitalWrite(M3_IN1, LOW);
        digitalWrite(M3_IN2, HIGH); }
    else {
        digitalWrite(M3_IN1, LOW);
        digitalWrite(M3_IN2, LOW); }
}

// Move M3 to target ADC position using closed-loop feedback
// Reads potentiometer on BACK_ADC to determine current position
// Stops when within M3_POS_TOL or after M3_TIMEOUT_MS
static void moveM3To(int target) {
    unsigned long start = millis();
    while (millis() - start < M3_TIMEOUT_MS) {
        int pos = analogRead(BACK_ADC);
        int err = target - pos;
        Serial.printf("[MOTOR] M3 pos=%d target=%d\n", pos, target);
        if (abs(err) <= M3_POS_TOL) break;
        applyM3(err > 0 ? 1 : -1);
        delay(10);
    }
    applyM3(0);  // stop motor after reaching target or timeout
}

// ── Public API ──────────────────────────────────────────────

// Initialize all motor pins and set to stopped state
void Motor::init() {
    pinMode(M1_IN1, OUTPUT); pinMode(M1_IN2, OUTPUT);
    pinMode(M2_IN1, OUTPUT); pinMode(M2_IN2, OUTPUT);
    pinMode(M3_IN1, OUTPUT); pinMode(M3_IN2, OUTPUT);
    applyM1(0); applyM2(0); applyM3(0);
    Serial.printf("[MOTOR] M3 init pos=%d\n", analogRead(BACK_ADC));
}

// Full-speed drive forward/backward (speed=200)
void Motor::runForward()  { g_speed = 200; applyM1(1);  applyM2(1);  }
void Motor::runBackward() { g_speed = 200; applyM1(-1); applyM2(-1); }

// Stop all motors (M1, M2, M3)
void Motor::stop()        { applyM1(0); applyM2(0); applyM3(0); }

// Spin in place: M1 and M2 drive in opposite directions
void Motor::turnLeft()    { applyM1(-1); applyM2(1);  }
void Motor::turnRight()   { applyM1(1);  applyM2(-1); }

// Walk forward: alternate legs 5 steps each (M1 step → M2 step → ...)
void Motor::walkForward() {
    g_speed = 150;
    for (int i = 0; i < 5; i++) {
        applyM1(1);  applyM2(0);  delay(WALK_STEP_MS);  // left leg step
        applyM1(0);  applyM2(1);  delay(WALK_STEP_MS);  // right leg step
    }
    applyM1(0); applyM2(0);  // stop after completing gait
}

// Walk backward: alternate legs 5 steps each
void Motor::walkBackward() {
    g_speed = 150;
    for (int i = 0; i < 5; i++) {
        applyM1(-1); applyM2(0);  delay(WALK_STEP_MS);  // left leg step back
        applyM1(0);  applyM2(-1); delay(WALK_STEP_MS);  // right leg step back
    }
    applyM1(0); applyM2(0);
}

// Posture control via M3 (stop M1/M2 first to prevent sliding)
void Motor::sit()   { stop(); moveM3To(M3_POS_SIT);   }
void Motor::stand() { stop(); moveM3To(M3_POS_STAND);  }
void Motor::lie()   { stop(); moveM3To(M3_POS_LIE);    }

// Dance sequence: stand → fwd/bwd → turn → sit → stand
void Motor::dance() {
    stand();
    runForward();  delay(400);
    runBackward(); delay(400);
    turnLeft();    delay(300);
    turnRight();   delay(300);
    stop();
    sit();         delay(500);
    stand();
}

// Increase speed by 50 PWM, keep current direction
void Motor::speedUp() {
    g_speed = (g_speed <= 205) ? g_speed + 50 : 255;
    applyM1(g_dirM1);
    applyM2(g_dirM2);
}

// Decrease speed by 50 PWM, keep current direction
void Motor::slowDown() {
    g_speed = (g_speed >= 50) ? g_speed - 50 : 0;
    applyM1(g_dirM1);
    applyM2(g_dirM2);
}
