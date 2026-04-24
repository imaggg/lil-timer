#include "ui.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "obs_client.h"
#include <string.h>

static TimerPreset presets[MAX_PRESETS];
static AppSettings settings;
static UIState ui;
static lilka::Canvas canvas;
static bool prev_wifi_enabled = false;
static uint8_t prev_ext_btn_pin_idx = 0xFF; // force init on first loop

// --- External button state ---
static int extBtnGpioPin = -1;
static bool extBtnLast = HIGH;
static unsigned long extBtnPressMs = 0;
static int extBtnClicks = 0;
static unsigned long extBtnReleaseMs = 0;

#define EXT_BTN_DEBOUNCE_MS 20
#define EXT_BTN_DOUBLE_MS   400

static void extBtnInit(uint8_t idx)
{
    if (extBtnGpioPin >= 0)
        pinMode(extBtnGpioPin, INPUT);
    extBtnGpioPin = (idx == 0) ? -1 : (int)EXT_BTN_PINS[idx - 1];
    if (extBtnGpioPin >= 0)
        pinMode(extBtnGpioPin, INPUT_PULLUP);
    extBtnLast = HIGH;
    extBtnPressMs = 0;
    extBtnClicks = 0;
    extBtnReleaseMs = 0;
}

static void extBtnUpdate(UIState &ui)
{
    if (extBtnGpioPin < 0 || ui.screen != SCREEN_TIMER)
        return;

    bool cur = (bool)digitalRead(extBtnGpioPin);
    unsigned long now = millis();
    TimerEngine &eng = ui.timer_engine;

    // Falling edge → press
    if (extBtnLast == HIGH && cur == LOW)
    {
        extBtnPressMs = now;
    }

    // Rising edge → release
    if (extBtnLast == LOW && cur == HIGH)
    {
        unsigned long held = now - extBtnPressMs;
        if (held >= EXT_BTN_DEBOUNCE_MS)
        {
            extBtnClicks++;
            extBtnReleaseMs = now;
        }
    }

    // Dispatch pending clicks after double-click window
    if (extBtnClicks > 0 && cur == HIGH &&
        now - extBtnReleaseMs >= EXT_BTN_DOUBLE_MS)
    {
        if (extBtnClicks >= 2)
        {
            timerSkipStep(eng);
        }
        else
        {
            if (eng.state == TIMER_READY)        timerStart(eng);
            else if (eng.state == TIMER_RUNNING) timerPause(eng);
            else if (eng.state == TIMER_PAUSED)  timerResume(eng);
        }
        extBtnClicks = 0;
    }

    extBtnLast = cur;
}

void setup() {
    lilka::begin();

    // Enable auto-repeat for L/R (8 repeats/sec after 400ms hold)
    lilka::controller.setAutoRepeat(lilka::Button::LEFT, 8, 400);
    lilka::controller.setAutoRepeat(lilka::Button::RIGHT, 8, 400);

    storageInit();
    storageLoadSettings(settings);
    storageLoadAllPresets(presets);

    uiInit(ui);

    extBtnInit(settings.ext_btn_pin_idx);
    prev_ext_btn_pin_idx = settings.ext_btn_pin_idx;

    // Start WiFi if enabled at boot
    if (settings.wifi_enabled) {
        wifiStart(settings);
        webServerStart(presets, settings, ui);
        prev_wifi_enabled = true;
    }
}

void loop() {
    // Sync UI flags from settings
    ui.lang_uk = settings.lang_uk;
    ui.swap_ab = settings.swap_ab;

    // Handle external button pin changes
    if (settings.ext_btn_pin_idx != prev_ext_btn_pin_idx) {
        extBtnInit(settings.ext_btn_pin_idx);
        prev_ext_btn_pin_idx = settings.ext_btn_pin_idx;
    }

    extBtnUpdate(ui);

    // Handle WiFi enable/disable changes
    if (settings.wifi_enabled != prev_wifi_enabled) {
        if (settings.wifi_enabled) {
            wifiStart(settings);
            webServerStart(presets, settings, ui);
        } else {
            webServerStop();
            wifiStop();
        }
        prev_wifi_enabled = settings.wifi_enabled;
    }

    // Handle web server requests & AP/STA transitions
    if (settings.wifi_enabled) {
        wifiUpdate();
        webServerHandle();
    }

    lilka::State input = lilka::controller.getState();

    // Swap A and B buttons if setting enabled
    if (settings.swap_ab) {
        lilka::ButtonState temp = input.a;
        input.a = input.b;
        input.b = temp;
    }

    // Button C: toggle OBS scene (works on menu and timer screens)
    if (input.c.justPressed &&
        (ui.screen == SCREEN_MENU || ui.screen == SCREEN_TIMER)) {
        ui.obs_scene = (ui.obs_scene == 1) ? 2 : 1;
        if (settings.wifi_enabled && wifiIsSTAConnected() && settings.obs_host[0] != '\0') {
            const char* scene = (ui.obs_scene == 1) ? settings.obs_scene1 : settings.obs_scene2;
            obsSetScene(settings.obs_host, settings.obs_port, settings.obs_pass, scene);
        }
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
        case SCREEN_DMAX_TIMER:
            uiUpdateDmaxTimer(ui, input, settings.volume);
            break;
        case SCREEN_DMAX_EDITOR: {
            AppScreen prev = ui.screen;
            uiUpdateDmaxEditor(ui, input, presets[DMAX_PRESET_INDEX]);
            if (ui.screen != prev) {
                storageSavePreset(DMAX_PRESET_INDEX, presets[DMAX_PRESET_INDEX]);
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
        case SCREEN_DMAX_TIMER:
            uiDrawDmaxTimer(canvas, ui, settings.brightness);
            break;
        case SCREEN_DMAX_EDITOR:
            uiDrawDmaxEditor(canvas, ui, presets[DMAX_PRESET_INDEX], settings.brightness);
            break;
    }

    lilka::display.drawCanvas(&canvas);
}
