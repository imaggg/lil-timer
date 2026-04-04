#pragma once

#include "timer.h"

enum AppScreen {
    SCREEN_MENU,
    SCREEN_PRESET_SELECT,
    SCREEN_PRESET_EDITOR,
    SCREEN_TIMER,
    SCREEN_SETTINGS
};

enum PresetSelectIntent {
    INTENT_START_TIMER,
    INTENT_EDIT_PRESET
};

enum EditorField {
    FIELD_NONE,
    FIELD_LABEL,
    FIELD_MINUTES,
    FIELD_SECONDS,
    FIELD_SOUND
};

struct UIState {
    AppScreen screen;
    int menu_cursor;
    int preset_cursor;
    PresetSelectIntent preset_intent;

    int editor_cursor;
    EditorField editor_field;

    bool editing_preset_name;
    int name_char_pos;

    int settings_cursor;

    bool lang_uk;   // mirror of AppSettings::lang_uk for draw functions
    bool swap_ab;   // mirror of AppSettings::swap_ab for draw functions

    TimerEngine timer_engine;
};

void uiInit(UIState& ui);

void uiDrawMenu(lilka::Canvas& c, UIState& ui, uint8_t bright);
void uiUpdateMenu(UIState& ui, lilka::State& input);

void uiDrawPresetSelect(lilka::Canvas& c, UIState& ui, TimerPreset presets[MAX_PRESETS], uint8_t bright);
void uiUpdatePresetSelect(UIState& ui, lilka::State& input, TimerPreset presets[MAX_PRESETS]);

void uiDrawPresetEditor(lilka::Canvas& c, UIState& ui, TimerPreset& preset, uint8_t bright);
void uiUpdatePresetEditor(UIState& ui, lilka::State& input, TimerPreset& preset);

void uiDrawTimer(lilka::Canvas& c, UIState& ui, uint8_t bright);
void uiUpdateTimer(UIState& ui, lilka::State& input);

void uiDrawSettings(lilka::Canvas& c, UIState& ui, AppSettings& settings, uint8_t bright);
void uiUpdateSettings(UIState& ui, lilka::State& input, AppSettings& settings);
