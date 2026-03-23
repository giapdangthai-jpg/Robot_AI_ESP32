#include "config_store.h"
#include <Preferences.h>

// Compile-time defaults (used when NVS has no saved value yet)
static constexpr const char* DEFAULT_WS_HOST = "192.168.100.170";
static constexpr int         DEFAULT_WS_PORT = 8000;
static constexpr const char* DEFAULT_WS_PATH = "/ws/public";
static constexpr int         DEFAULT_VOLUME  = 50;

// RAM cache — populated by load()
static char g_wsHost[64];
static int  g_wsPort;
static int  g_volume;

void ConfigStore::load() {
    Preferences prefs;
    prefs.begin("robot", /*readOnly=*/true);
    String host = prefs.getString("ws_host", DEFAULT_WS_HOST);
    g_wsPort    = prefs.getInt   ("ws_port", DEFAULT_WS_PORT);
    g_volume    = prefs.getInt   ("volume",  DEFAULT_VOLUME);
    prefs.end();

    strncpy(g_wsHost, host.c_str(), sizeof(g_wsHost) - 1);
    g_wsHost[sizeof(g_wsHost) - 1] = '\0';

    Serial.printf("[CFG] ws_host=%s  ws_port=%d  volume=%d\n", g_wsHost, g_wsPort, g_volume);
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
int         ConfigStore::volume() { return g_volume; }

void ConfigStore::saveVolume(int vol) {
    if (vol < 0)   vol = 0;
    if (vol > 100) vol = 100;
    g_volume = vol;
    Preferences prefs;
    prefs.begin("robot", /*readOnly=*/false);
    prefs.putInt("volume", g_volume);
    prefs.end();
    Serial.printf("[CFG] volume=%d\n", g_volume);
}
