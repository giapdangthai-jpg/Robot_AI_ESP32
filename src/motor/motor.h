#pragma once

class Motor {
public:
    static void init();
    static void runForward();
    static void runBackward();
    static void stop();
    static void turnLeft();
    static void turnRight();
    static void walkForward();
    static void walkBackward();
    static void sit();
    static void stand();
    static void lie();
    static void dance();
    static void speedUp();
    static void slowDown();
};
