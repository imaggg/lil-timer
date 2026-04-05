#pragma once

#include "config.h"
#include "storage.h"

void wifiStart(const AppSettings& settings);
void wifiStop();
bool wifiIsRunning();
String wifiGetAPIP();
String wifiGetSTAIP();
bool wifiIsSTAConnected();
