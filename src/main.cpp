#include "ui.h"

static TimerPreset presets[MAX_PRESETS];
static AppSettings settings;
static UIState ui;
static lilka::Canvas canvas;

void setup() {
    lilka::begin();

    // Enable auto-repeat for L/R (8 repeats/sec after 400ms hold)
    lilka::controller.setAutoRepeat(lilka::Button::LEFT, 8, 400);
    lilka::controller.setAutoRepeat(lilka::Button::RIGHT, 8, 400);

    storageInit();
    storageLoadSettings(settings);
    storageLoadAllPresets(presets);

    uiInit(ui);
}

void loop() {
    lilka::State input = lilka::controller.getState();

    // Update
    switch (ui.screen) {
        case SCREEN_MENU:
            uiUpdateMenu(ui, input);
            break;
        case SCREEN_PRESET_SELECT:
            uiUpdatePresetSelect(ui, input, presets);
            break;
        case SCREEN_PRESET_EDITOR: {
            AppScreen prev = ui.screen;
            uiUpdatePresetEditor(ui, input, presets[ui.preset_cursor]);
            if (ui.screen != prev) {
                storageSavePreset(ui.preset_cursor, presets[ui.preset_cursor]);
            }
            break;
        }
        case SCREEN_TIMER:
            ui.timer_engine.volume = settings.volume;
            uiUpdateTimer(ui, input);
            break;
        case SCREEN_SETTINGS: {
            AppSettings prev = settings;
            uiUpdateSettings(ui, input, settings);
            if (ui.screen != SCREEN_SETTINGS ||
                prev.brightness != settings.brightness ||
                prev.volume != settings.volume) {
                storageSaveSettings(settings);
            }
            break;
        }
    }

    // Draw
    switch (ui.screen) {
        case SCREEN_MENU:
            uiDrawMenu(canvas, ui, settings.brightness);
            break;
        case SCREEN_PRESET_SELECT:
            uiDrawPresetSelect(canvas, ui, presets, settings.brightness);
            break;
        case SCREEN_PRESET_EDITOR:
            uiDrawPresetEditor(canvas, ui, presets[ui.preset_cursor], settings.brightness);
            break;
        case SCREEN_TIMER:
            uiDrawTimer(canvas, ui, settings.brightness);
            break;
        case SCREEN_SETTINGS:
            uiDrawSettings(canvas, ui, settings, settings.brightness);
            break;
    }

    lilka::display.drawCanvas(&canvas);
}
