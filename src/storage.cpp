#include "storage.h"
#include <Preferences.h>
#include <string.h>

static Preferences prefs;

static void setDefaultPreset0(TimerPreset& p) {
    strncpy(p.name, "BW Film", MAX_PRESET_NAME);
    p.name[MAX_PRESET_NAME] = '\0';
    p.step_count = 3;

    strncpy(p.steps[0].label, "DEV", MAX_LABEL_LEN);
    p.steps[0].duration_sec = 300;
    p.steps[0].end_sound_enabled = true;

    strncpy(p.steps[1].label, "STOP", MAX_LABEL_LEN);
    p.steps[1].duration_sec = 30;
    p.steps[1].end_sound_enabled = true;

    strncpy(p.steps[2].label, "FIX", MAX_LABEL_LEN);
    p.steps[2].duration_sec = 300;
    p.steps[2].end_sound_enabled = true;
}

void storageInit() {
    prefs.begin("dt_settings", true);
    bool initialized = prefs.getBool("init", false);
    prefs.end();

    if (!initialized) {
        TimerPreset presets[MAX_PRESETS];
        AppSettings settings;
        settings.brightness = DEFAULT_BRIGHTNESS;
        settings.volume = DEFAULT_VOLUME;
        settings.lang_uk = false;
        settings.swap_ab = false;

        for (int i = 0; i < MAX_PRESETS; i++) {
            memset(&presets[i], 0, sizeof(TimerPreset));
            char name[MAX_PRESET_NAME + 1];
            snprintf(name, sizeof(name), "Preset %d", i + 1);
            strncpy(presets[i].name, name, MAX_PRESET_NAME);
            presets[i].name[MAX_PRESET_NAME] = '\0';
            presets[i].step_count = 0;
        }
        setDefaultPreset0(presets[0]);

        storageSaveSettings(settings);
        for (int i = 0; i < MAX_PRESETS; i++) {
            storageSavePreset(i, presets[i]);
        }

        prefs.begin("dt_settings", false);
        prefs.putBool("init", true);
        prefs.end();
    }
}

void storageLoadSettings(AppSettings& settings) {
    prefs.begin("dt_settings", true);
    settings.brightness = prefs.getUChar("bright", DEFAULT_BRIGHTNESS);
    settings.volume = prefs.getUChar("volume", DEFAULT_VOLUME);
    settings.lang_uk = prefs.getBool("lang_uk", false);
    settings.swap_ab = prefs.getBool("swap_ab", false);
    prefs.end();

    if (settings.brightness >= BRIGHTNESS_COUNT) settings.brightness = DEFAULT_BRIGHTNESS;
    if (settings.volume >= VOLUME_COUNT) settings.volume = DEFAULT_VOLUME;
}

void storageSaveSettings(const AppSettings& settings) {
    prefs.begin("dt_settings", false);
    prefs.putUChar("bright", settings.brightness);
    prefs.putUChar("volume", settings.volume);
    prefs.putBool("lang_uk", settings.lang_uk);
    prefs.putBool("swap_ab", settings.swap_ab);
    prefs.end();
}

void storageLoadPreset(uint8_t index, TimerPreset& preset) {
    if (index >= MAX_PRESETS) return;

    char ns[16];
    snprintf(ns, sizeof(ns), "dt_preset_%d", index);
    prefs.begin(ns, true);

    char defName[MAX_PRESET_NAME + 1];
    snprintf(defName, sizeof(defName), "Preset %d", index + 1);
    String name = prefs.getString("name", defName);
    strncpy(preset.name, name.c_str(), MAX_PRESET_NAME);
    preset.name[MAX_PRESET_NAME] = '\0';

    preset.step_count = prefs.getUChar("count", 0);
    if (preset.step_count > MAX_STEPS) preset.step_count = 0;

    for (int i = 0; i < preset.step_count; i++) {
        char key[8];
        snprintf(key, sizeof(key), "s%d_lbl", i);
        String lbl = prefs.getString(key, "DEV");
        strncpy(preset.steps[i].label, lbl.c_str(), MAX_LABEL_LEN);
        preset.steps[i].label[MAX_LABEL_LEN] = '\0';

        snprintf(key, sizeof(key), "s%d_dur", i);
        preset.steps[i].duration_sec = prefs.getUShort(key, 60);

        snprintf(key, sizeof(key), "s%d_snd", i);
        preset.steps[i].end_sound_enabled = prefs.getBool(key, true);
    }

    prefs.end();
}

void storageSavePreset(uint8_t index, const TimerPreset& preset) {
    if (index >= MAX_PRESETS) return;

    char ns[16];
    snprintf(ns, sizeof(ns), "dt_preset_%d", index);
    prefs.begin(ns, false);

    prefs.putString("name", preset.name);
    prefs.putUChar("count", preset.step_count);

    for (int i = 0; i < preset.step_count; i++) {
        char key[8];
        snprintf(key, sizeof(key), "s%d_lbl", i);
        prefs.putString(key, preset.steps[i].label);

        snprintf(key, sizeof(key), "s%d_dur", i);
        prefs.putUShort(key, preset.steps[i].duration_sec);

        snprintf(key, sizeof(key), "s%d_snd", i);
        prefs.putBool(key, preset.steps[i].end_sound_enabled);
    }

    prefs.end();
}

void storageLoadAllPresets(TimerPreset presets[MAX_PRESETS]) {
    for (int i = 0; i < MAX_PRESETS; i++) {
        storageLoadPreset(i, presets[i]);
    }
}
