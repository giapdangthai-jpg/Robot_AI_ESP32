#include "wifi_mgr.h"
#include "../config/secrets.h"
#include <WiFi.h>

void WifiMgr::connect() {

    WiFi.mode(WIFI_STA);

    Serial.println("[WiFi] Connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < 10000) {

        Serial.print(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("[WiFi] Connected!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] RSSI: ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println();
        Serial.println("[WiFi] Failed to connect!");
    }
}

