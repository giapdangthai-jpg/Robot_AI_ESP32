// #include <Arduino.h>
// #include <Adafruit_NeoPixel.h>

// #define RGB_PIN 48          // WS2812 onboard ESP32-S3 thường dùng pin này

// Adafruit_NeoPixel rgb(1, RGB_PIN, NEO_GRB + NEO_KHZ800);
// void setup() {
//   Serial.begin(115200);
//   delay(2000);
//   Serial.println("Robot Dog Boot OK");

//   rgb.begin();
//   rgb.setBrightness(10);   // dịu mắt
// }

// void loop() {
//   Serial.println("Running...");

//   rgb.setPixelColor(0, rgb.Color(50, 0, 0)); // đỏ
//   rgb.show();
//   delay(1000);
  

//   rgb.setPixelColor(0, rgb.Color(0, 50, 0)); // xanh lá
//   rgb.show();
//   delay(1000);

//   rgb.setPixelColor(0, rgb.Color(0, 0, 50)); // xanh dương
//   rgb.show();
//   delay(1000);
// }

#include <Arduino.h>

#include "core/system.h"
#include "core/event_bus.h"
#include "motor/motor.h"

void startMotorTask();
void startWebSocketTask();
void startAudioTask();

void setup() {

    System::init();
    EventBus::init();
    Motor::init();

    startMotorTask();
    startWebSocketTask();
    startAudioTask();

    Serial.println("Robot RTOS Ready");
}

void loop() {
    vTaskDelay(1000);
}
