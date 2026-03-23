// ── RobotAI Entry Point ──────────────────────────────────────
// setup(): initialize hardware and start all FreeRTOS tasks
// loop():  button polling + serial commands

#include <Arduino.h>

#include "core/system.h"
#include "core/event_bus.h"
#include "motor/motor.h"
#include "motor/motor_task.h"
#include "network/websocket_mgr.h"
#include "network/wifi_mgr.h"
#include "utils/rgb_led.h"
#include "config/pinmap.h"
#include "config/config_store.h"
#include "audio/speaker_i2s.h"
#include "../test/hello_from_gabi_pcm.h"

extern WebSocketMgr wsmgr;

void setup() {
    // Init Serial, I2S, WiFi, WebSocket, audio tasks, mic stream
    System::init();
    EventBus::init();
    Motor::init();
    startMotorTask();

    // wsmgr.setConnectCallback([]() {
    //     bool ok = wsmgr.sendHelloFromGabiAudio();
    //     Serial.println(ok ? "[APP] Hello from gabi audio sent" : "[APP] Hello from gabi audio failed");
    // });

    delay(2000);
    Serial.println("Robot RTOS Ready");
}

// ── BOOT button WiFi reset ────────────────────────────────────
// Hold BOOT (GPIO0) for 3s any time while running → erase WiFi + server
// config → restart into setup portal.

static unsigned long s_bootPressStart  = 0;
static bool          s_bootInProgress  = false;
// Require the button to be seen HIGH at least once before detecting a hold.
// Prevents false-trigger when GPIO0 is already LOW at boot (strapping pin behavior).
static bool          s_bootSeenHigh    = false;

static void pollWifiResetButton() {
    bool pressed = (digitalRead(BOOT_BTN_PIN) == LOW);

    // Wait until button has been released at least once
    if (!s_bootSeenHigh) {
        if (!pressed) s_bootSeenHigh = true;
        return;
    }

    if (pressed) {
        if (!s_bootInProgress) {
            s_bootInProgress = true;
            s_bootPressStart = millis();
            Serial.println("[BTN] BOOT pressed — hold 3s to reset WiFi...");
        }

        unsigned long held = millis() - s_bootPressStart;
        // Purple blink every 300ms as progress indicator
        bool blink = (held / 300) % 2;
        RgbLed::setColor(blink ? 80 : 0, 0, blink ? 80 : 0);

        if (held >= WIFI_RESET_HOLD_MS) {
            Serial.println("[BTN] WiFi reset triggered!");
            RgbLed::setRed();
            WifiMgr::resetAndProvision();  // erases NVS + restarts (no return)
        }
    } else {
        if (s_bootInProgress) {
            s_bootInProgress = false;
            Serial.println("[BTN] BOOT released — reset cancelled");
            // Restore correct LED colour based on connection state
            if (wsmgr.isConnected()) RgbLed::setBlue();
            else                     RgbLed::setGreen();
        }
    }
}

// ── Serial command tester ─────────────────────────────────────
// Open Serial Monitor (115200), type shorthand → publishes to EventBus
// Commands: f b l r s sit stand lie dance + -

static void pollSerialCommand() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // Prefix 'text' → send text to server as {"type":"ask","text":"..."}
    if (cmd.startsWith("text")) {
        String text = cmd.substring(4);
        text.trim();
        if (text.length() > 0) {
            bool ok = wsmgr.sendMessage(text.c_str());
            Serial.printf("[TEST] send text: \"%s\" %s\n", text.c_str(), ok ? "ok" : "FAIL");
        }
        return;
    }

    EventType t = EventType::NONE;
    if      (cmd == "f")     t = EventType::CMD_RUN_FORWARD;
    else if (cmd == "b")     t = EventType::CMD_RUN_BACKWARD;
    else if (cmd == "l")     t = EventType::CMD_TURN_LEFT;
    else if (cmd == "r")     t = EventType::CMD_TURN_RIGHT;
    else if (cmd == "s")     t = EventType::CMD_STOP;
    else if (cmd == "sit")   t = EventType::CMD_SIT;
    else if (cmd == "stand") t = EventType::CMD_STAND;
    else if (cmd == "lie")   t = EventType::CMD_LIE;
    else if (cmd == "dance") t = EventType::CMD_DANCE;
    else if (cmd == "+")     t = EventType::CMD_SPEED_UP;
    else if (cmd == "-")     t = EventType::CMD_SLOW_DOWN;
    else if (cmd == "hello") {
        size_t offset = 0;
        const unsigned long kTimeoutMs = 8000;
        unsigned long startMs = millis();
        bool ok = true;
        while (offset < HELLO_FROM_GABI_PCM_LEN) {
            size_t chunk = min((size_t)AUDIO_FRAME_BYTES, HELLO_FROM_GABI_PCM_LEN - offset);
            if (wsmgr.sendBinary(HELLO_FROM_GABI_PCM + offset, chunk)) {
                offset += chunk;
            } else if (millis() - startMs > kTimeoutMs) {
                ok = false; break;
            } else {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        if (ok) {
            unsigned long endMs = millis();
            while (!wsmgr.sendText("{\"type\":\"audio_end\"}")) {
                if (millis() - endMs > kTimeoutMs) { ok = false; break; }
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        Serial.println(ok ? "[APP] Hello from gabi sent" : "[APP] Hello from gabi FAILED");
        return;
    }

    // Volume commands: vol+ / vol- / vol N (0–100)
    if (cmd == "vol+") {
        SpeakerI2S::setVolume(min(100, SpeakerI2S::getVolume() + 10));
        Serial.printf("[VOL] %d\n", SpeakerI2S::getVolume());
        return;
    }
    if (cmd == "vol-") {
        SpeakerI2S::setVolume(max(0, SpeakerI2S::getVolume() - 10));
        Serial.printf("[VOL] %d\n", SpeakerI2S::getVolume());
        return;
    }
    if (cmd.startsWith("vol ")) {
        int v = cmd.substring(4).toInt();
        SpeakerI2S::setVolume(v);
        Serial.printf("[VOL] %d\n", SpeakerI2S::getVolume());
        return;
    }

    if (t != EventType::NONE) {
        EventBus::publish({t, 0});
        Serial.printf("[TEST] cmd=%s\n", cmd.c_str());
    } else {
        Serial.println("[TEST] unknown. use: f b l r s sit stand lie dance + - hello vol+ vol- vol N");
    }
}

// ── Main loop ─────────────────────────────────────────────────

void loop() {
    pollWifiResetButton();
    pollSerialCommand();
    delay(100);  // 100ms loop → button responsive, low CPU overhead
}
