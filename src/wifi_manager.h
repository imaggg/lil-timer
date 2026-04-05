#pragma once

#include "config.h"
#include "storage.h"

void wifiStart(const AppSettings& settings);
void wifiStop();
void wifiUpdate(); // call in loop — manages AP/STA transitions
bool wifiIsRunning();
String wifiGetIP();  // returns STA IP if connected, else AP IP
bool wifiIsSTAConnected();
