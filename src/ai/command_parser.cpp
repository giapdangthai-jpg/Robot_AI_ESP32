#include "command_parser.h"
#include <string.h>
#include <ctype.h>

// Returns true if `haystack` contains `needle` (case-insensitive, ASCII only)
static bool contains(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;

    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen == 0 || nlen > hlen) return false;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        bool match = true;
        for (size_t j = 0; j < nlen; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

// Keyword table — first match wins, so more specific phrases go first
EventType CommandParser::parse(const char* text) {
    if (!text) return EventType::NONE;

    // ── Movement ─────────────────────────────────────────────────

    // Run forward
    if (contains(text, "chạy tiến") || contains(text, "run forward") ||
        contains(text, "chạy về phía trước"))
        return EventType::CMD_RUN_FORWARD;

    // Run backward
    if (contains(text, "chạy lùi") || contains(text, "run backward") ||
        contains(text, "chạy về phía sau"))
        return EventType::CMD_RUN_BACKWARD;

    // Walk forward — check after "chạy" to avoid false match
    if (contains(text, "đi tiến") || contains(text, "walk forward") ||
        contains(text, "đi về phía trước") || contains(text, "tiến lên"))
        return EventType::CMD_WALK_FORWARD;

    // Walk backward
    if (contains(text, "đi lùi") || contains(text, "walk backward") ||
        contains(text, "đi về phía sau") || contains(text, "lùi lại"))
        return EventType::CMD_WALK_BACKWARD;

    // Turn left
    if (contains(text, "quay trái") || contains(text, "rẽ trái") ||
        contains(text, "turn left"))
        return EventType::CMD_TURN_LEFT;

    // Turn right
    if (contains(text, "quay phải") || contains(text, "rẽ phải") ||
        contains(text, "turn right"))
        return EventType::CMD_TURN_RIGHT;

    // Stop
    if (contains(text, "dừng lại") || contains(text, "đứng lại") ||
        contains(text, "stop") || contains(text, "dừng"))
        return EventType::CMD_STOP;

    // Dance
    if (contains(text, "nhảy đi") || contains(text, "nhảy") ||
        contains(text, "dance") || contains(text, "múa"))
        return EventType::CMD_DANCE;

    // Speed up
    if (contains(text, "nhanh hơn") || contains(text, "tăng tốc") ||
        contains(text, "speed up") || contains(text, "faster"))
        return EventType::CMD_SPEED_UP;

    // Slow down
    if (contains(text, "chậm hơn") || contains(text, "giảm tốc") ||
        contains(text, "slow down") || contains(text, "slower"))
        return EventType::CMD_SLOW_DOWN;

    // ── Posture ───────────────────────────────────────────────────

    // Sit
    if (contains(text, "ngồi xuống") || contains(text, "ngồi") ||
        contains(text, "sit down") || contains(text, "sit"))
        return EventType::CMD_SIT;

    // Stand
    if (contains(text, "đứng dậy") || contains(text, "đứng") ||
        contains(text, "stand up") || contains(text, "stand"))
        return EventType::CMD_STAND;

    // Lie down
    if (contains(text, "nằm xuống") || contains(text, "nằm") ||
        contains(text, "lie down") || contains(text, "lay down"))
        return EventType::CMD_LIE;

    // ── Audio ─────────────────────────────────────────────────────

    // Volume up
    if (contains(text, "to hơn") || contains(text, "tăng âm") ||
        contains(text, "volume up") || contains(text, "louder"))
        return EventType::CMD_VOLUME_UP;

    // Volume down
    if (contains(text, "nhỏ hơn") || contains(text, "giảm âm") ||
        contains(text, "volume down") || contains(text, "quieter"))
        return EventType::CMD_VOLUME_DOWN;

    return EventType::NONE;
}
