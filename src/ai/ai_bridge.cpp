#include "ai_bridge.h"
#include <Arduino.h>

void AiBridge::handleAnswer(const char* text)
{
    Serial.println("===== AI RESPONSE =====");
    Serial.println(text);
    Serial.println("=======================");

    // TODO:
    // - gửi sang TTS
    // - parse command nếu có
    // - gửi event qua EventBus
}
