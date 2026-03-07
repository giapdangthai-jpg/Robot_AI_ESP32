#include "config_store.h"
#include <Preferences.h>

// Compile-time defaults (used when NVS has no saved value yet)
static constexpr const char* DEFAULT_WS_HOST = "10.10.10.139";
static constexpr int         DEFAULT_WS_PORT = 8000;
static constexpr const char* DEFAULT_WS_PATH = "/ws/public";

// RAM cache — populated by load(), read by wsHost()/wsPort()
static char g_wsHost[64];
static int  g_wsPort;

void ConfigStore::load() {
    Preferences prefs;
    prefs.begin("robot", /*readOnly=*/true);
    String host = prefs.getString("ws_host", DEFAULT_WS_HOST);
    g_wsPort    = prefs.getInt   ("ws_port", DEFAULT_WS_PORT);
    prefs.end();

    strncpy(g_wsHost, host.c_str(), sizeof(g_wsHost) - 1);
    g_wsHost[sizeof(g_wsHost) - 1] = '\0';

    Serial.printf("[CFG] ws_host=%s  ws_port=%d\n", g_wsHost, g_wsPort);
}

void ConfigStore::saveWs(const char* host, int port) {
    Preferences prefs;
    prefs.begin("robot", /*readOnly=*/false);
    prefs.putString("ws_host", host);
    prefs.putInt   ("ws_port", port);
    prefs.end();

    // Update RAM cache immediately
    strncpy(g_wsHost, host, sizeof(g_wsHost) - 1);
    g_wsHost[sizeof(g_wsHost) - 1] = '\0';
    g_wsPort = port;

    Serial.printf("[CFG] Saved ws_host=%s  ws_port=%d\n", g_wsHost, g_wsPort);
}

const char* ConfigStore::wsHost() { return g_wsHost; }
int         ConfigStore::wsPort() { return g_wsPort; }
const char* ConfigStore::wsPath() { return DEFAULT_WS_PATH; }
