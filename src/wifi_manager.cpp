#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>

static bool running = false;
static bool ap_active = false;

void wifiStart(const AppSettings& settings) {
    if (running) return;

    // Always start with AP so user can reach the web UI
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    ap_active = true;

    if (settings.wifi_ssid[0] != '\0') {
        WiFi.begin(settings.wifi_ssid, settings.wifi_pass);
    }

    MDNS.begin(WIFI_HOSTNAME);
    running = true;
}

void wifiStop() {
    if (!running) return;
    MDNS.end();
    WiFi.disconnect(true);
    if (ap_active) WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    running = false;
    ap_active = false;
}

void wifiUpdate() {
    if (!running) return;

    bool sta_connected = (WiFi.status() == WL_CONNECTED);

    if (sta_connected && ap_active) {
        // STA connected — disable AP, switch to STA-only
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        ap_active = false;
    } else if (!sta_connected && !ap_active) {
        // STA lost — re-enable AP so user can reach web UI
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
        ap_active = true;
    }
}

bool wifiIsRunning() {
    return running;
}

String wifiGetIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

bool wifiIsSTAConnected() {
    return WiFi.status() == WL_CONNECTED;
}
