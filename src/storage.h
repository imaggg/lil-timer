#pragma once

#include "config.h"

struct TimerStep {
    char label[MAX_LABEL_LEN + 1];
    uint16_t duration_sec;
    bool end_sound_enabled;
};

struct TimerPreset {
    char name[MAX_PRESET_NAME + 1];
    TimerStep steps[MAX_STEPS];
    uint8_t step_count;
};

struct AppSettings {
    uint8_t brightness;
    uint8_t volume;
    bool lang_uk;       // false = EN, true = UK
    bool swap_ab;       // swap A and B buttons
    bool wifi_enabled;
    char wifi_ssid[33]; // STA SSID (empty = no STA)
    char wifi_pass[65]; // STA password
};

void storageInit();
void storageLoadSettings(AppSettings& settings);
void storageSaveSettings(const AppSettings& settings);
void storageLoadPreset(uint8_t index, TimerPreset& preset);
void storageSavePreset(uint8_t index, const TimerPreset& preset);
void storageLoadAllPresets(TimerPreset presets[MAX_PRESETS]);
