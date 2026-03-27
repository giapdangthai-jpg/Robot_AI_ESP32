#pragma once
#include <stdint.h>

// WS2812 RGB LED status indicator (GPIO48 on ESP32-S3 DevKit)
// Red   → no WiFi
// Green → WiFi connected, WebSocket disconnected
// Blue  → WiFi + WebSocket connected
class RgbLed {
public:
    static void init();

    static void setRed();    // no WiFi
    static void setGreen();  // WiFi connected
    static void setBlue();   // WebSocket connected, idle
    static void setOrange(); // speech detected — server is listening
    static void setPurple(); // thinking — STT/LLM processing

    static void setColor(uint8_t r, uint8_t g, uint8_t b);
    static void off();
};
