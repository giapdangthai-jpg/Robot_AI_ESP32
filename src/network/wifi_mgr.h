#pragma once

class WifiMgr {
public:
    // Connect to WiFi. Blocks until connected.
    // Opens SoftAP "RobotAI-Setup" for browser-based config on first boot.
    static void connect();

    // Erase saved credentials and restart into provisioning mode.
    static void resetAndProvision();
};
