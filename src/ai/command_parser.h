#pragma once
#include "../core/event.h"

// Parses free-form LLM text response and maps it to an EventType command.
// Returns EventType::NONE if no recognizable command is found.
class CommandParser {
public:
    static EventType parse(const char* text);
};
