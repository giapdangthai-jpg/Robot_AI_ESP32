#include "ai_bridge.h"
#include "command_parser.h"
#include "../core/event_bus.h"
#include <Arduino.h>

// Called when the server returns a text response from the LLM.
// user_text (STT transcript) is tried first for command detection — it's shorter and more
// command-like than the LLM rephrasing. Falls back to llm_text if user_text is absent.
void AiBridge::handleAnswer(const char* llm_text, const char* user_text)
{
    Serial.println("===== AI RESPONSE =====");
    Serial.println(llm_text);
    Serial.println("=======================");

    // Prefer STT text for command parsing (more direct), fall back to LLM text
    const char* src = (user_text && user_text[0] != '\0') ? user_text : llm_text;

    EventType cmd = CommandParser::parse(src);
    if (cmd == EventType::NONE && src != llm_text) {
        // Also try the LLM response in case it rephrases the command
        cmd = CommandParser::parse(llm_text);
    }

    if (cmd != EventType::NONE) {
        Event e { cmd, 0 };
        if (!EventBus::publish(e)) {
            Serial.println("[AI] EventBus full — command dropped");
        }
    }
}
