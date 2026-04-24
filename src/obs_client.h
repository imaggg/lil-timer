#pragma once
#include <stdint.h>

// Connect to OBS WebSocket v5 and set the current program scene.
// Synchronous, blocks up to ~3s. Returns true on success.
// Pass empty string for password if OBS auth is disabled.
bool obsSetScene(const char* host, uint16_t port, const char* password, const char* scene_name);
