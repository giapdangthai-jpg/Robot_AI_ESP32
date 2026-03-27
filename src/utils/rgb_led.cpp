#include "rgb_led.h"
#include "../config/pinmap.h"
#include <Adafruit_NeoPixel.h>

// ESP32-S3 DevKit has 1 WS2812 LED on RGB_PIN
static Adafruit_NeoPixel g_led(1, RGB_PIN, NEO_GRB + NEO_KHZ800);

void RgbLed::init() {
    g_led.begin();
    g_led.setBrightness(10);  // 0-255; keep dim to avoid distracting
    setRed();                  // default: no WiFi
}

void RgbLed::setRed()   { setColor(255, 0,   0);   }
void RgbLed::setGreen() { setColor(0,   255, 0);   }
void RgbLed::setBlue()  { setColor(0,   0,   255); }
void RgbLed::setOrange() { setColor(255, 80, 0);   }
void RgbLed::off()      { setColor(0,   0,   0);   }

void RgbLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    g_led.setPixelColor(0, g_led.Color(r, g, b));
    g_led.show();
}
