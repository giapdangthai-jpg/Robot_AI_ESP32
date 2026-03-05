// ── RobotAI Entry Point ──────────────────────────────────────
// setup(): initialize hardware and start all FreeRTOS tasks
// loop():  minimal; all work is done by tasks (audio, WS, motor)

#include <Arduino.h>

#include "core/system.h"
#include "core/event_bus.h"
#include "motor/motor.h"
#include "motor/motor_task.h"
#include "network/websocket_mgr.h"

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

void loop() {
    static unsigned long lastText = 0;
    static bool sentHelloAudio = false;

    // if (millis() - lastText > 15000)
    // {
    //     wsmgr.sendText("Hello from ESP");
    //     lastText = millis();
    // }
    
    delay(2000);

    // Send test audio once after WebSocket connects
    if (!sentHelloAudio && wsmgr.isConnected()) {
        bool ok = wsmgr.sendHelloFromGabiAudio();
        Serial.println(ok ? "[APP] Hello from gabi audio sent" : "[APP] Hello from gabi audio failed");
        sentHelloAudio = ok;
    }

    delay(200);
}
