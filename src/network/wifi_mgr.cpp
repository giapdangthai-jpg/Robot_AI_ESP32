#include "wifi_mgr.h"
#include "../config/config_store.h"
#include "../utils/rgb_led.h"
#include <WiFiManager.h>
#include <Arduino.h>

// AP name shown on phone's WiFi list during provisioning
static constexpr const char* PROVISION_AP_NAME = "RobotAI-Setup";

// Blocking: returns only after WiFi is connected.
// - First boot (no saved credentials): starts SoftAP "RobotAI-Setup".
//   User opens 192.168.4.1 in browser, picks WiFi, enters password,
//   and optionally sets the WebSocket server IP/port.
// - Subsequent boots: connects automatically using saved NVS credentials.
// - If connection fails: re-opens SoftAP for reconfiguration.
void WifiMgr::connect() {
    // Load ws_host/ws_port from NVS so portal shows current values as defaults
    ConfigStore::load();

    WiFiManager wm;
    wm.setDebugOutput(false);
    wm.setConfigPortalTimeout(180);  // 3-minute portal timeout then restart

    // Custom fields shown in the setup portal alongside WiFi credentials
    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", ConfigStore::wsPort());

    WiFiManagerParameter wsHostParam("ws_host", "Server IP",   ConfigStore::wsHost(), 63);
    WiFiManagerParameter wsPortParam("ws_port", "Server Port", portStr,               7);
    wm.addParameter(&wsHostParam);
    wm.addParameter(&wsPortParam);

    Serial.println("[WiFi] Starting WiFiManager...");

    if (!wm.autoConnect(PROVISION_AP_NAME)) {
        Serial.println("[WiFi] Config portal timed out — restarting");
        ESP.restart();
    }

    // Persist server settings entered by user (no-op if unchanged)
    int port = String(wsPortParam.getValue()).toInt();
    if (port <= 0 || port > 65535) port = 8000;
    ConfigStore::saveWs(wsHostParam.getValue(), port);

    RgbLed::setGreen();  // WiFi connected, waiting for WebSocket
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] SSID: ");  Serial.println(WiFi.SSID());
    Serial.print("[WiFi] IP:   ");  Serial.println(WiFi.localIP());
    Serial.print("[WiFi] RSSI: ");  Serial.println(WiFi.RSSI());
}

// Erase saved WiFi + NVS credentials and restart into provisioning mode.
// Useful when moving to a new network or changing the server IP.
void WifiMgr::resetAndProvision() {
    WiFiManager wm;
    wm.resetSettings();
    Serial.println("[WiFi] Credentials erased — restarting into setup mode");
    ESP.restart();
}
