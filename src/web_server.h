#pragma once

#include "storage.h"

void webServerStart(TimerPreset presets[], AppSettings& settings);
void webServerStop();
void webServerHandle();
bool webServerIsRunning();
