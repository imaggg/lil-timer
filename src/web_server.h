#pragma once

#include "ui.h"

void webServerStart(TimerPreset presets[], AppSettings& settings, UIState& ui);
void webServerStop();
void webServerHandle();
bool webServerIsRunning();
