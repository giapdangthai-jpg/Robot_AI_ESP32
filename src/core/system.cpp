#include "system.h"
#include "../network/wifi_mgr.h"
#include "../config/pinmap.h"
#include <Arduino.h>

WifiMgr wifi;

void System::init() {
    Serial.begin(DEBUG_BAUD);
    delay(500);
    Serial.println("System init...");
    wifi.connect();
}
