#include "wifi_mgr.h"
#include "../config/secrets.h"
#include <WiFi.h>

void WifiMgr::connect() {
    WiFi.mode(WIFI_STA);
    
    Serial.println("[WiFi] Scanning for networks...");
    int n = WiFi.scanNetworks();
    Serial.println("[WiFi] Scan complete: " + String(n));
    
    if (n == 0) {
        Serial.println("[WiFi] No networks found");
        return;
    }
    
    // Try each configured network
    for (int i = 0; i < WIFI_COUNT; i++) {
        Serial.print("[WiFi] Trying to connect to: ");
        Serial.println(WIFI_SSIDS[i]);
        
        // Check if this network is available
        bool networkFound = false;
        for (int j = 0; j < n; j++) {
            if (WiFi.SSID(j) == WIFI_SSIDS[i]) {
                networkFound = true;
                Serial.print("[WiFi] Network found with RSSI: ");
                Serial.println(WiFi.RSSI(j));
                break;
            }
        }
        
        if (networkFound) {
            WiFi.begin(WIFI_SSIDS[i], WIFI_PASSWORDS[i]);
            
            // Wait for connection (no timeout - wait until connected)
            unsigned long startAttemptTime = millis();
            Serial.print("[WiFi] Connecting");
            while (WiFi.status() != WL_CONNECTED &&
                    millis() - startAttemptTime < 10000) {
                Serial.print(".");
                delay(500);
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println();
                Serial.println("[WiFi] Connected successfully!");
                Serial.print("[WiFi] SSID: ");
                Serial.println(WiFi.SSID());
                Serial.print("[WiFi] IP: ");
                Serial.println(WiFi.localIP());
                Serial.print("[WiFi] RSSI: ");
                Serial.println(WiFi.RSSI());
                Serial.print("[WiFi] Gateway: ");
                Serial.println(WiFi.gatewayIP());
                Serial.print("[WiFi] Subnet: ");
                Serial.println(WiFi.subnetMask());
            } else {
            Serial.println();
            Serial.println("[WiFi] Failed to connect!");
            }
            
            return;
        } else {
            Serial.println("[WiFi] Network not found in scan");
        }
    }
    
    Serial.println("[WiFi] No configured networks found!");
}

