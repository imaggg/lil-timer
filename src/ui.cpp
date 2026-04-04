#include "ui.h"
#include <stdio.h>
#include <string.h>

static void formatTime(uint16_t sec, char* buf, size_t len) {
    snprintf(buf, len, "%02d:%02d", sec / 60, sec % 60);
}

void uiInit(UIState& ui) {
    ui.screen = SCREEN_MENU;
    ui.menu_cursor = 0;
    ui.preset_cursor = 0;
    ui.preset_intent = INTENT_START_TIMER;
    ui.editor_cursor = 0;
    ui.editor_field = FIELD_NONE;
    ui.editing_preset_name = false;
    ui.name_char_pos = 0;
    ui.settings_cursor = 0;
}

// =====================
//     MAIN MENU
// =====================
static const char* MENU_ITEMS[] = {"Start", "Edit", "Settings"};
#define MENU_ITEM_COUNT 3

void uiDrawMenu(lilka::Canvas& c, UIState& ui, uint8_t bright) {
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);

    // Title
    c.setFont(FONT_10x20);
    c.setTextSize(2);
    c.drawTextAligned("DARKROOM", SCREEN_W / 2, 30, lilka::ALIGN_CENTER, lilka::ALIGN_START);
    c.setTextColor(fg);
    c.drawTextAligned("TIMER", SCREEN_W / 2, 65, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    c.setTextSize(1);
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        int16_t y = 110 + i * 30;
        if (i == ui.menu_cursor) {
            c.fillRect(50, y - 5, 180, 25, hl);
        }
        c.setTextColor(i == ui.menu_cursor ? fg : dim);
        char buf[20];
        snprintf(buf, sizeof(buf), "%s%s", i == ui.menu_cursor ? "> " : "  ", MENU_ITEMS[i]);
        c.setCursor(60, y + 12);
        c.print(buf);
    }

    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 10);
    c.print("[A] Select");
}

void uiUpdateMenu(UIState& ui, lilka::State& input) {
    if (input.up.justPressed) {
        ui.menu_cursor = (ui.menu_cursor - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
    }
    if (input.down.justPressed) {
        ui.menu_cursor = (ui.menu_cursor + 1) % MENU_ITEM_COUNT;
    }
    if (input.a.justPressed) {
        switch (ui.menu_cursor) {
            case 0:
                ui.preset_intent = INTENT_START_TIMER;
                ui.preset_cursor = 0;
                ui.screen = SCREEN_PRESET_SELECT;
                break;
            case 1:
                ui.preset_intent = INTENT_EDIT_PRESET;
                ui.preset_cursor = 0;
                ui.screen = SCREEN_PRESET_SELECT;
                break;
            case 2:
                ui.settings_cursor = 0;
                ui.screen = SCREEN_SETTINGS;
                break;
        }
    }
}

// =====================
//    PRESET SELECT
// =====================
void uiDrawPresetSelect(lilka::Canvas& c, UIState& ui, TimerPreset presets[MAX_PRESETS], uint8_t bright) {
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);

    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    const char* title = (ui.preset_intent == INTENT_START_TIMER) ? "Select Preset" : "Edit Preset";
    c.drawTextAligned(title, SCREEN_W / 2, 10, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    c.setFont(FONT_8x13);
    for (int i = 0; i < MAX_PRESETS; i++) {
        int16_t y = 42 + i * 36;
        if (i == ui.preset_cursor) {
            c.fillRect(15, y - 3, SCREEN_W - 30, 32, hl);
        }
        c.setTextColor(i == ui.preset_cursor ? fg : dim);
        c.setCursor(25, y + 10);
        if (i == ui.preset_cursor) c.print("> ");
        else c.print("  ");
        c.print(presets[i].name);

        if (presets[i].step_count > 0) {
            c.setTextColor(dim);
            c.setCursor(45, y + 24);
            for (int s = 0; s < presets[i].step_count && s < 5; s++) {
                if (s > 0) c.print(">");
                c.print(presets[i].steps[s].label);
            }
            if (presets[i].step_count > 5) c.print("...");
        }
    }

    c.setTextColor(dim);
    c.setFont(FONT_6x12);
    c.setCursor(20, SCREEN_H - 10);
    c.print("[A] Select  [B] Back");
}

void uiUpdatePresetSelect(UIState& ui, lilka::State& input, TimerPreset presets[MAX_PRESETS]) {
    if (input.up.justPressed) {
        ui.preset_cursor = (ui.preset_cursor - 1 + MAX_PRESETS) % MAX_PRESETS;
    }
    if (input.down.justPressed) {
        ui.preset_cursor = (ui.preset_cursor + 1) % MAX_PRESETS;
    }
    if (input.b.justPressed) {
        ui.screen = SCREEN_MENU;
        return;
    }
    if (input.a.justPressed) {
        if (ui.preset_intent == INTENT_START_TIMER) {
            if (presets[ui.preset_cursor].step_count == 0) return;
            timerInit(ui.timer_engine, &presets[ui.preset_cursor], 0);
            ui.screen = SCREEN_TIMER;
        } else {
            ui.editor_cursor = 0;
            ui.editor_field = FIELD_NONE;
            ui.editing_preset_name = false;
            ui.screen = SCREEN_PRESET_EDITOR;
        }
    }
}

// =====================
//    PRESET EDITOR
// =====================
static int findLabelIndex(const char* label) {
    for (int i = 0; i < STEP_LABEL_COUNT; i++) {
        if (strcmp(label, STEP_LABELS[i]) == 0) return i;
    }
    return 0;
}

void uiDrawPresetEditor(lilka::Canvas& c, UIState& ui, TimerPreset& preset, uint8_t bright) {
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);

    c.setFont(FONT_8x13);
    c.setTextSize(1);

    // Preset name
    c.setTextColor(fg);
    c.setCursor(20, 15);
    c.print("Preset: ");
    if (ui.editing_preset_name) {
        c.print("[");
        c.print(preset.name);
        c.print("]");
    } else {
        c.print(preset.name);
    }

    // Steps list
    int max_visible = 7;
    int visible_start = 0;
    if (ui.editor_cursor >= max_visible) {
        visible_start = ui.editor_cursor - max_visible + 1;
    }

    for (int i = visible_start; i < preset.step_count && (i - visible_start) < max_visible; i++) {
        int16_t y = 32 + (i - visible_start) * 24;
        bool sel = (i == ui.editor_cursor && !ui.editing_preset_name);

        if (sel) {
            c.fillRect(15, y - 2, SCREEN_W - 30, 20, hl);
        }

        TimerStep& step = preset.steps[i];
        char timeBuf[8];
        formatTime(step.duration_sec, timeBuf, sizeof(timeBuf));

        // Label
        c.setTextColor((sel && ui.editor_field == FIELD_LABEL) ? fg : dim);
        c.setCursor(20, y + 12);
        if (sel && ui.editor_field == FIELD_LABEL) {
            c.print("["); c.print(step.label); c.print("]");
        } else {
            c.print(step.label);
        }

        // Time
        bool editMins = sel && ui.editor_field == FIELD_MINUTES;
        bool editSecs = sel && ui.editor_field == FIELD_SECONDS;
        c.setTextColor((editMins || editSecs) ? fg : (sel ? fg : dim));
        c.setCursor(80, y + 12);
        if (editMins) {
            char buf[16];
            snprintf(buf, sizeof(buf), "[%02d]:%02d", step.duration_sec / 60, step.duration_sec % 60);
            c.print(buf);
        } else if (editSecs) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:[%02d]", step.duration_sec / 60, step.duration_sec % 60);
            c.print(buf);
        } else {
            c.print(timeBuf);
        }

        // Sound
        c.setCursor(180, y + 12);
        if (sel && ui.editor_field == FIELD_SOUND) {
            c.setTextColor(fg);
            c.print(step.end_sound_enabled ? "[SND]" : "[---]");
        } else {
            c.setTextColor(step.end_sound_enabled ? dim : hl);
            c.print(step.end_sound_enabled ? "SND" : "---");
        }
    }

    if (preset.step_count == 0) {
        c.setTextColor(dim);
        c.setCursor(20, 80);
        c.print("No steps. Press [C] to add.");
    }

    // Hints
    c.setFont(FONT_6x12);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 24);
    c.print("[A] Edit  [C] Add  [D] Del");
    c.setCursor(20, SCREEN_H - 10);
    c.print("[B] Save  [START] Name");
}

void uiUpdatePresetEditor(UIState& ui, lilka::State& input, TimerPreset& preset) {
    // Name editing
    if (ui.editing_preset_name) {
        if (input.left.justPressed) {
            char& ch = preset.name[ui.name_char_pos];
            if (ch <= ' ' || ch > '~') ch = '~'; else ch--;
        }
        if (input.right.justPressed) {
            char& ch = preset.name[ui.name_char_pos];
            if (ch >= '~' || ch < ' ') ch = ' '; else ch++;
        }
        if (input.up.justPressed && ui.name_char_pos > 0) ui.name_char_pos--;
        if (input.down.justPressed && ui.name_char_pos < MAX_PRESET_NAME - 1) {
            int len = strlen(preset.name);
            if (ui.name_char_pos < len) {
                ui.name_char_pos++;
                if (ui.name_char_pos >= len) {
                    preset.name[ui.name_char_pos] = ' ';
                    preset.name[ui.name_char_pos + 1] = '\0';
                }
            }
        }
        if (input.a.justPressed || input.start.justPressed) {
            int len = strlen(preset.name);
            while (len > 0 && preset.name[len - 1] == ' ') preset.name[--len] = '\0';
            if (len == 0) strncpy(preset.name, "Preset", MAX_PRESET_NAME);
            ui.editing_preset_name = false;
        }
        if (input.b.justPressed) ui.editing_preset_name = false;
        return;
    }

    // Field editing
    if (ui.editor_field != FIELD_NONE) {
        TimerStep& step = preset.steps[ui.editor_cursor];

        if (input.left.justPressed) {
            switch (ui.editor_field) {
                case FIELD_LABEL: {
                    int idx = findLabelIndex(step.label);
                    idx = (idx - 1 + STEP_LABEL_COUNT) % STEP_LABEL_COUNT;
                    strncpy(step.label, STEP_LABELS[idx], MAX_LABEL_LEN);
                    step.label[MAX_LABEL_LEN] = '\0';
                    break;
                }
                case FIELD_MINUTES: {
                    int m = step.duration_sec / 60, s = step.duration_sec % 60;
                    m = m > 0 ? m - 1 : 99;
                    step.duration_sec = m * 60 + s;
                    if (step.duration_sec < MIN_DURATION_SEC) step.duration_sec = MIN_DURATION_SEC;
                    if (step.duration_sec > MAX_DURATION_SEC) step.duration_sec = MAX_DURATION_SEC;
                    break;
                }
                case FIELD_SECONDS: {
                    int m = step.duration_sec / 60, s = step.duration_sec % 60;
                    s = s > 0 ? s - 1 : 59;
                    step.duration_sec = m * 60 + s;
                    if (step.duration_sec < MIN_DURATION_SEC) step.duration_sec = MIN_DURATION_SEC;
                    break;
                }
                case FIELD_SOUND:
                    step.end_sound_enabled = !step.end_sound_enabled;
                    break;
                default: break;
            }
        }
        if (input.right.justPressed) {
            switch (ui.editor_field) {
                case FIELD_LABEL: {
                    int idx = findLabelIndex(step.label);
                    idx = (idx + 1) % STEP_LABEL_COUNT;
                    strncpy(step.label, STEP_LABELS[idx], MAX_LABEL_LEN);
                    step.label[MAX_LABEL_LEN] = '\0';
                    break;
                }
                case FIELD_MINUTES: {
                    int m = step.duration_sec / 60, s = step.duration_sec % 60;
                    m = m < 99 ? m + 1 : 0;
                    step.duration_sec = m * 60 + s;
                    if (step.duration_sec < MIN_DURATION_SEC) step.duration_sec = MIN_DURATION_SEC;
                    if (step.duration_sec > MAX_DURATION_SEC) step.duration_sec = MAX_DURATION_SEC;
                    break;
                }
                case FIELD_SECONDS: {
                    int m = step.duration_sec / 60, s = step.duration_sec % 60;
                    s = s < 59 ? s + 1 : 0;
                    step.duration_sec = m * 60 + s;
                    if (step.duration_sec < MIN_DURATION_SEC) step.duration_sec = MIN_DURATION_SEC;
                    break;
                }
                case FIELD_SOUND:
                    step.end_sound_enabled = !step.end_sound_enabled;
                    break;
                default: break;
            }
        }
        if (input.a.justPressed) {
            switch (ui.editor_field) {
                case FIELD_LABEL:   ui.editor_field = FIELD_MINUTES; break;
                case FIELD_MINUTES: ui.editor_field = FIELD_SECONDS; break;
                case FIELD_SECONDS: ui.editor_field = FIELD_SOUND; break;
                case FIELD_SOUND:   ui.editor_field = FIELD_NONE; break;
                default: break;
            }
        }
        if (input.b.justPressed) ui.editor_field = FIELD_NONE;
        return;
    }

    // Navigation
    if (input.up.justPressed && preset.step_count > 0) {
        ui.editor_cursor = (ui.editor_cursor - 1 + preset.step_count) % preset.step_count;
    }
    if (input.down.justPressed && preset.step_count > 0) {
        ui.editor_cursor = (ui.editor_cursor + 1) % preset.step_count;
    }
    if (input.a.justPressed && preset.step_count > 0) {
        ui.editor_field = FIELD_LABEL;
    }
    if (input.start.justPressed) {
        ui.editing_preset_name = true;
        ui.name_char_pos = 0;
    }
    if (input.c.justPressed && preset.step_count < MAX_STEPS) {
        TimerStep& ns = preset.steps[preset.step_count];
        strncpy(ns.label, "DEV", MAX_LABEL_LEN);
        ns.label[MAX_LABEL_LEN] = '\0';
        ns.duration_sec = 60;
        ns.end_sound_enabled = true;
        preset.step_count++;
        ui.editor_cursor = preset.step_count - 1;
    }
    if (input.d.justPressed && preset.step_count > 0) {
        for (int i = ui.editor_cursor; i < preset.step_count - 1; i++) {
            preset.steps[i] = preset.steps[i + 1];
        }
        preset.step_count--;
        if (ui.editor_cursor >= preset.step_count && ui.editor_cursor > 0) {
            ui.editor_cursor--;
        }
    }
    if (input.b.justPressed) {
        ui.screen = SCREEN_MENU;
    }
}

// =====================
//    TIMER RUNNING
// =====================
void uiDrawTimer(lilka::Canvas& c, UIState& ui, uint8_t bright) {
    c.fillScreen(COLOR_BG);
    TimerEngine& eng = ui.timer_engine;

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);

    if (eng.state == TIMER_DONE) {
        c.setFont(FONT_10x20);
        c.setTextSize(2);
        c.setTextColor(fg);
        c.drawTextAligned("ALL DONE", SCREEN_W / 2, 80, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);
        c.setTextColor(dim);
        c.drawTextAligned("[B] Back to menu", SCREEN_W / 2, 150, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        return;
    }

    TimerStep& step = eng.preset->steps[eng.current_step];

    // Step label + progress
    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    c.setCursor(20, 25);
    c.print(step.label);

    char progBuf[10];
    snprintf(progBuf, sizeof(progBuf), "%d / %d", eng.current_step + 1, eng.preset->step_count);
    c.setCursor(SCREEN_W - 85, 25);
    c.print(progBuf);

    // State
    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, 45);
    switch (eng.state) {
        case TIMER_READY:   c.print("READY"); break;
        case TIMER_RUNNING: c.print("RUNNING"); break;
        case TIMER_PAUSED:  c.print("PAUSED"); break;
        default: break;
    }

    // Large countdown
    char timeBuf[8];
    formatTime(eng.remaining_sec, timeBuf, sizeof(timeBuf));
    c.setFont(FONT_10x20);
    c.setTextSize(3);
    c.setTextColor(fg);
    c.drawTextAligned(timeBuf, SCREEN_W / 2, 90, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
    c.setTextSize(1);

    // Progress bar
    uint16_t total = step.duration_sec;
    uint16_t elapsed = total - eng.remaining_sec;
    int barW = SCREEN_W - 60;
    int barX = 30;
    int barY = 150;
    int barH = 8;
    c.drawRect(barX, barY, barW, barH, dim);
    if (total > 0) {
        int fillW = (int)((long)elapsed * (barW - 2) / total);
        if (fillW > 0) {
            c.fillRect(barX + 1, barY + 1, fillW, barH - 2, fg);
        }
    }

    // Next step preview
    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    if (eng.current_step + 1 < eng.preset->step_count) {
        TimerStep& next = eng.preset->steps[eng.current_step + 1];
        char nextTime[8];
        formatTime(next.duration_sec, nextTime, sizeof(nextTime));
        c.setCursor(20, 178);
        c.print("next: ");
        c.print(next.label);
        c.print("  ");
        c.print(nextTime);
    } else {
        c.setCursor(20, 178);
        c.print("last step");
    }

    // Hints
    c.setFont(FONT_6x12);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 10);
    switch (eng.state) {
        case TIMER_READY:   c.print("[A] Start  [B] Back"); break;
        case TIMER_RUNNING: c.print("[B] Pause  [START] Skip"); break;
        case TIMER_PAUSED:  c.print("[A] Resume  [B] Back"); break;
        default: break;
    }
}

void uiUpdateTimer(UIState& ui, lilka::State& input) {
    TimerEngine& eng = ui.timer_engine;
    timerUpdate(eng);

    switch (eng.state) {
        case TIMER_READY:
            if (input.a.justPressed) timerStart(eng);
            if (input.b.justPressed) ui.screen = SCREEN_MENU;
            break;
        case TIMER_RUNNING:
            if (input.b.justPressed) timerPause(eng);
            if (input.start.justPressed) timerSkipStep(eng);
            break;
        case TIMER_PAUSED:
            if (input.a.justPressed) timerResume(eng);
            if (input.b.justPressed) ui.screen = SCREEN_MENU;
            break;
        case TIMER_DONE:
            // Auto-reset handled by timerUpdate after 3 sec
            // Manual: B goes to menu, A restarts immediately
            if (input.b.justPressed) ui.screen = SCREEN_MENU;
            if (input.a.justPressed) timerReset(eng);
            break;
    }
}

// =====================
//      SETTINGS
// =====================
static const char* VOLUME_NAMES[] = {"OFF", "LOW", "MED", "HIGH"};

void uiDrawSettings(lilka::Canvas& c, UIState& ui, AppSettings& settings, uint8_t bright) {
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);

    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    c.drawTextAligned("Settings", SCREEN_W / 2, 10, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    c.setFont(FONT_8x13);
    // Brightness
    int16_t y0 = 60;
    if (ui.settings_cursor == 0) c.fillRect(15, y0 - 5, SCREEN_W - 30, 25, hl);
    c.setTextColor(ui.settings_cursor == 0 ? fg : dim);
    c.setCursor(25, y0 + 10);
    c.print("Brightness  ");
    for (int b = 0; b < BRIGHTNESS_COUNT; b++) {
        c.print(b <= settings.brightness ? "#" : "-");
    }

    // Volume
    int16_t y1 = 100;
    if (ui.settings_cursor == 1) c.fillRect(15, y1 - 5, SCREEN_W - 30, 25, hl);
    c.setTextColor(ui.settings_cursor == 1 ? fg : dim);
    c.setCursor(25, y1 + 10);
    c.print("Volume      ");
    c.print(VOLUME_NAMES[settings.volume]);

    // Hints
    c.setFont(FONT_6x12);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 24);
    c.print("[L/R] Adjust");
    c.setCursor(20, SCREEN_H - 10);
    c.print("[B] Back");
}

void uiUpdateSettings(UIState& ui, lilka::State& input, AppSettings& settings) {
    if (input.up.justPressed) ui.settings_cursor = (ui.settings_cursor + 1) % 2;
    if (input.down.justPressed) ui.settings_cursor = (ui.settings_cursor + 1) % 2;

    if (input.left.justPressed) {
        if (ui.settings_cursor == 0 && settings.brightness > 0) settings.brightness--;
        if (ui.settings_cursor == 1 && settings.volume > 0) settings.volume--;
    }
    if (input.right.justPressed) {
        if (ui.settings_cursor == 0 && settings.brightness < BRIGHTNESS_COUNT - 1) settings.brightness++;
        if (ui.settings_cursor == 1 && settings.volume < VOLUME_COUNT - 1) settings.volume++;
    }
    if (input.b.justPressed) ui.screen = SCREEN_MENU;
}
