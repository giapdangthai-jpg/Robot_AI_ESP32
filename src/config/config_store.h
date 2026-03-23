#pragma once
#include <Arduino.h>

// Persistent configuration stored in NVS (survives restarts).
// Load once at boot; save when user changes settings via WiFiManager portal.
class ConfigStore {
public:
    // Load all config from NVS into RAM. Call before first use.
    static void load();

    // WebSocket server settings
    static const char* wsHost();
    static int         wsPort();
    static const char* wsPath();

    // Persist ws_host and ws_port to NVS
    static void saveWs(const char* host, int port);

    // Speaker volume: 0 = mute, 100 = full (persisted in NVS)
    static int  volume();
    static void saveVolume(int vol);
};
