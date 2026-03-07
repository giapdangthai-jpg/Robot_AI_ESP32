// ── RobotAI Entry Point ──────────────────────────────────────
// setup(): initialize hardware and start all FreeRTOS tasks
// loop():  button polling + one-time test audio send

#include <Arduino.h>

#include "core/system.h"
#include "core/event_bus.h"
#include "motor/motor.h"
#include "motor/motor_task.h"
#include "network/websocket_mgr.h"
#include "network/wifi_mgr.h"
#include "utils/rgb_led.h"
#include "config/pinmap.h"

extern WebSocketMgr wsmgr;

void setup() {
    // Init Serial, I2S, WiFi, WebSocket, audio tasks, mic stream
    System::init();
    EventBus::init();
    Motor::init();
    startMotorTask();

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

// ── Main loop ─────────────────────────────────────────────────

void loop() {
    static bool sentHelloAudio = false;

    pollWifiResetButton();

    // Send test audio once after WebSocket connects
    if (!sentHelloAudio && wsmgr.isConnected()) {
        bool ok = wsmgr.sendHelloFromGabiAudio();
        Serial.println(ok ? "[APP] Hello from gabi audio sent" : "[APP] Hello from gabi audio failed");
        sentHelloAudio = ok;
    }

    delay(100);  // 100ms loop → button responsive, low CPU overhead
}
