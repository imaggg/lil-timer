#include "ui.h"
#include "wifi_manager.h"
#include "web_server.h"
#include <string.h>

static TimerPreset presets[MAX_PRESETS];
static AppSettings settings;
static UIState ui;
static lilka::Canvas canvas;
static bool prev_wifi_enabled = false;

void setup() {
    lilka::begin();

    // Enable auto-repeat for L/R (8 repeats/sec after 400ms hold)
    lilka::controller.setAutoRepeat(lilka::Button::LEFT, 8, 400);
    lilka::controller.setAutoRepeat(lilka::Button::RIGHT, 8, 400);

    storageInit();
    storageLoadSettings(settings);
    storageLoadAllPresets(presets);

    uiInit(ui);

    // Start WiFi if enabled at boot
    if (settings.wifi_enabled) {
        wifiStart(settings);
        webServerStart(presets, settings);
        prev_wifi_enabled = true;
    }
}

void loop() {
    // Sync UI flags from settings
    ui.lang_uk = settings.lang_uk;
    ui.swap_ab = settings.swap_ab;

    // Handle WiFi enable/disable changes
    if (settings.wifi_enabled != prev_wifi_enabled) {
        if (settings.wifi_enabled) {
            wifiStart(settings);
            webServerStart(presets, settings);
        } else {
            webServerStop();
            wifiStop();
        }
        prev_wifi_enabled = settings.wifi_enabled;
    }

    // Handle web server requests
    if (settings.wifi_enabled) {
        webServerHandle();
    }

    lilka::State input = lilka::controller.getState();

    // Swap A and B buttons if setting enabled
    if (settings.swap_ab) {
        lilka::ButtonState temp = input.a;
        input.a = input.b;
        input.b = temp;
    }

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
                memcmp(&prev, &settings, sizeof(AppSettings)) != 0) {
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
