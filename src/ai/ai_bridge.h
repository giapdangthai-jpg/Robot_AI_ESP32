#pragma once

class AiBridge {
public:
    // llm_text: LLM response (printed to Serial)
    // user_text: original STT transcript (preferred source for command parsing)
    static void handleAnswer(const char* llm_text, const char* user_text = nullptr);
};
