#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>

static bool running = false;

void wifiStart(const AppSettings& settings) {
    if (running) return;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

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
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    running = false;
}

bool wifiIsRunning() {
    return running;
}

String wifiGetAPIP() {
    return WiFi.softAPIP().toString();
}

String wifiGetSTAIP() {
    return WiFi.localIP().toString();
}

bool wifiIsSTAConnected() {
    return WiFi.status() == WL_CONNECTED;
}
