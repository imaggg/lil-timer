#include "ui.h"
#include "wifi_manager.h"
#include <stdio.h>
#include <string.h>

static void formatTime(uint16_t sec, char *buf, size_t len)
{
    snprintf(buf, len, "%02d:%02d", sec / 60, sec % 60);
}

// Localization helper
static const char *L(bool uk, const char *en, const char *uk_str)
{
    return uk ? uk_str : en;
}

// Button label helpers for A/B swap
static const char *btnA(bool swap) { return swap ? "[B]" : "[A]"; }
static const char *btnB(bool swap) { return swap ? "[A]" : "[B]"; }

void uiInit(UIState &ui)
{
    ui.screen = SCREEN_MENU;
    ui.menu_cursor = 0;
    ui.preset_cursor = 0;
    ui.preset_intent = INTENT_START_TIMER;
    ui.editor_cursor = 0;
    ui.editor_field = FIELD_NONE;
    ui.editing_preset_name = false;
    ui.name_char_pos = 0;
    ui.settings_cursor = 0;
    ui.lang_uk = false;
    ui.swap_ab = false;
}

// =====================
//     MAIN MENU
// =====================

void uiDrawMenu(lilka::Canvas &c, UIState &ui, uint8_t bright)
{
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);
    bool uk = ui.lang_uk;

    // Battery level (top-right)
    int batt = lilka::battery.readLevel();
    if (batt >= 0)
    {
        c.setFont(FONT_8x13);
        c.setTextSize(1);
        c.setTextColor(dim);
        char battBuf[8];
        snprintf(battBuf, sizeof(battBuf), "%d%%", batt);
        c.setCursor(SCREEN_W - 60, 22);
        c.print(battBuf);
    }

    // Title
    c.setFont(FONT_10x20);
    c.setTextSize(2);
    c.setTextColor(fg);
    c.drawTextAligned(L(uk, "DARKROOM", "DARKROOM"), SCREEN_W / 2, 38, lilka::ALIGN_CENTER, lilka::ALIGN_START);
    c.drawTextAligned(L(uk, "TIMER", "TIMER"), SCREEN_W / 2, 73, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    // Menu items
    const char *MENU_ITEMS_EN[] = {"Start", "Edit", "Settings"};
    const char *MENU_ITEMS_UK[] = {"Старт", "Редактор", "Налаштування"};
    const char **items = uk ? MENU_ITEMS_UK : MENU_ITEMS_EN;

    c.setTextSize(1);
    c.setFont(FONT_10x20);
    for (int i = 0; i < 3; i++)
    {
        int16_t y = 118 + i * 32;
        if (i == ui.menu_cursor)
        {
            c.fillRect(20, y - 5, SCREEN_W - 40, 27, hl);
        }
        c.setTextColor(i == ui.menu_cursor ? fg : dim);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s%s", i == ui.menu_cursor ? "> " : "  ", items[i]);
        c.setCursor(30, y + 14);
        c.print(buf);
    }

    // WiFi info
    if (wifiIsRunning())
    {
        c.setFont(FONT_8x13);
        c.setTextColor(dim);
        String ip = wifiGetIP();
        c.setCursor(20, 22);
        c.print("WiFi: ");
        c.print(ip.c_str());
    }

    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 18);
    char hintBuf[32];
    snprintf(hintBuf, sizeof(hintBuf), "%s %s", btnA(ui.swap_ab), L(uk, "Select", "Обрати"));
    c.print(hintBuf);
}

void uiUpdateMenu(UIState &ui, lilka::State &input)
{
    if (input.up.justPressed)
    {
        ui.menu_cursor = (ui.menu_cursor - 1 + 3) % 3;
    }
    if (input.down.justPressed)
    {
        ui.menu_cursor = (ui.menu_cursor + 1) % 3;
    }
    if (input.a.justPressed)
    {
        switch (ui.menu_cursor)
        {
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
void uiDrawPresetSelect(lilka::Canvas &c, UIState &ui, TimerPreset presets[MAX_PRESETS], uint8_t bright)
{
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);
    bool uk = ui.lang_uk;

    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    const char *title;
    if (ui.preset_intent == INTENT_START_TIMER)
    {
        title = L(uk, "Select Preset", "Оберіть пресет");
    }
    else
    {
        title = L(uk, "Edit Preset", "Редагувати пресет");
    }
    c.drawTextAligned(title, SCREEN_W / 2, 18, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    c.setFont(FONT_9x15);
    for (int i = 0; i < MAX_PRESETS; i++)
    {
        int16_t y = 46 + i * 36;
        if (i == ui.preset_cursor)
        {
            c.fillRect(15, y - 3, SCREEN_W - 30, 33, hl);
        }
        c.setTextColor(i == ui.preset_cursor ? fg : dim);
        c.setCursor(25, y + 12);
        if (i == ui.preset_cursor)
            c.print("> ");
        else
            c.print("  ");
        c.print(presets[i].name);

        // Subtitle
        c.setTextColor(i == ui.preset_cursor ? fg : dim);
        c.setFont(FONT_8x13);
        c.setCursor(45, y + 26);
        if (i == DMAX_PRESET_INDEX)
        {
            char dbuf[16];
            snprintf(dbuf, sizeof(dbuf), "Delay: %ds", presets[i].steps[0].duration_sec);
            c.print(dbuf);
        }
        else if (presets[i].step_count > 0)
        {
            for (int s = 0; s < presets[i].step_count && s < 5; s++)
            {
                if (s > 0)
                    c.print(">");
                c.print(presets[i].steps[s].label);
            }
            if (presets[i].step_count > 5)
                c.print("...");
        }
        c.setFont(FONT_9x15);
    }

    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 18);
    char hintBuf[40];
    snprintf(hintBuf, sizeof(hintBuf), "%s %s  %s %s",
             btnA(ui.swap_ab), L(uk, "Select", "Обрати"),
             btnB(ui.swap_ab), L(uk, "Back", "Назад"));
    c.print(hintBuf);
}

void uiUpdatePresetSelect(UIState &ui, lilka::State &input, TimerPreset presets[MAX_PRESETS])
{
    if (input.up.justPressed)
    {
        ui.preset_cursor = (ui.preset_cursor - 1 + MAX_PRESETS) % MAX_PRESETS;
    }
    if (input.down.justPressed)
    {
        ui.preset_cursor = (ui.preset_cursor + 1) % MAX_PRESETS;
    }
    if (input.b.justPressed)
    {
        ui.screen = SCREEN_MENU;
        return;
    }
    if (input.a.justPressed)
    {
        if (ui.preset_cursor == DMAX_PRESET_INDEX)
        {
            if (ui.preset_intent == INTENT_START_TIMER)
            {
                ui.dmax_state = DMAX_READY;
                ui.dmax_delay = presets[DMAX_PRESET_INDEX].steps[0].duration_sec;
                if (ui.dmax_delay < 1)
                    ui.dmax_delay = DMAX_DEFAULT_DELAY;
                ui.screen = SCREEN_DMAX_TIMER;
            }
            else
            {
                ui.screen = SCREEN_DMAX_EDITOR;
            }
        }
        else if (ui.preset_intent == INTENT_START_TIMER)
        {
            if (presets[ui.preset_cursor].step_count == 0)
                return;
            timerInit(ui.timer_engine, &presets[ui.preset_cursor], 0);
            ui.screen = SCREEN_TIMER;
        }
        else
        {
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
static int findLabelIndex(const char *label)
{
    for (int i = 0; i < STEP_LABEL_COUNT; i++)
    {
        if (strcmp(label, STEP_LABELS[i]) == 0)
            return i;
    }
    return 0;
}

void uiDrawPresetEditor(lilka::Canvas &c, UIState &ui, TimerPreset &preset, uint8_t bright)
{
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);
    bool uk = ui.lang_uk;

    c.setFont(FONT_9x15);
    c.setTextSize(1);

    // Preset name
    c.setTextColor(fg);
    c.setCursor(20, 22);
    c.print(L(uk, "Preset: ", "Пресет: "));
    if (ui.editing_preset_name)
    {
        c.print("[");
        c.print(preset.name);
        c.print("]");
    }
    else
    {
        c.print(preset.name);
    }

    // Steps list
    int max_visible = 6;
    int visible_start = 0;
    if (ui.editor_cursor >= max_visible)
    {
        visible_start = ui.editor_cursor - max_visible + 1;
    }

    for (int i = visible_start; i < preset.step_count && (i - visible_start) < max_visible; i++)
    {
        int16_t y = 38 + (i - visible_start) * 26;
        bool sel = (i == ui.editor_cursor && !ui.editing_preset_name);

        if (sel)
        {
            c.fillRect(15, y - 2, SCREEN_W - 30, 22, hl);
        }

        TimerStep &step = preset.steps[i];
        char timeBuf[8];
        formatTime(step.duration_sec, timeBuf, sizeof(timeBuf));

        // Label
        c.setTextColor((sel && ui.editor_field == FIELD_LABEL) ? fg : dim);
        c.setCursor(20, y + 14);
        if (sel && ui.editor_field == FIELD_LABEL)
        {
            c.print("[");
            c.print(step.label);
            c.print("]");
        }
        else
        {
            c.print(step.label);
        }

        // Time
        bool editMins = sel && ui.editor_field == FIELD_MINUTES;
        bool editSecs = sel && ui.editor_field == FIELD_SECONDS;
        c.setTextColor((editMins || editSecs) ? fg : (sel ? fg : dim));
        c.setCursor(85, y + 14);
        if (editMins)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "[%02d]:%02d", step.duration_sec / 60, step.duration_sec % 60);
            c.print(buf);
        }
        else if (editSecs)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:[%02d]", step.duration_sec / 60, step.duration_sec % 60);
            c.print(buf);
        }
        else
        {
            c.print(timeBuf);
        }

        // Sound
        c.setCursor(190, y + 14);
        if (sel && ui.editor_field == FIELD_SOUND)
        {
            c.setTextColor(fg);
            c.print(step.end_sound_enabled ? "[SND]" : "[---]");
        }
        else
        {
            c.setTextColor(step.end_sound_enabled ? dim : hl);
            c.print(step.end_sound_enabled ? "SND" : "---");
        }
    }

    if (preset.step_count == 0)
    {
        c.setTextColor(dim);
        c.setCursor(20, 80);
        c.print(L(uk, "No steps. Press [C] to add.", "Немає кроків. [C] додати."));
    }

    // Hints
    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 32);
    char h1[40];
    snprintf(h1, sizeof(h1), "%s %s  [C] %s  [D] %s",
             btnA(ui.swap_ab), L(uk, "Edit", "Ред."),
             L(uk, "Add", "Дод."),
             L(uk, "Del", "Вид."));
    c.print(h1);
    c.setCursor(20, SCREEN_H - 18);
    char h2[40];
    snprintf(h2, sizeof(h2), "%s %s  [START] %s",
             btnB(ui.swap_ab), L(uk, "Save", "Зберег."),
             L(uk, "Name", "Назва"));
    c.print(h2);
}

void uiUpdatePresetEditor(UIState &ui, lilka::State &input, TimerPreset &preset)
{
    // Name editing
    if (ui.editing_preset_name)
    {
        if (input.left.justPressed)
        {
            char &ch = preset.name[ui.name_char_pos];
            if (ch <= ' ' || ch > '~')
                ch = '~';
            else
                ch--;
        }
        if (input.right.justPressed)
        {
            char &ch = preset.name[ui.name_char_pos];
            if (ch >= '~' || ch < ' ')
                ch = ' ';
            else
                ch++;
        }
        if (input.up.justPressed && ui.name_char_pos > 0)
            ui.name_char_pos--;
        if (input.down.justPressed && ui.name_char_pos < MAX_PRESET_NAME - 1)
        {
            int len = strlen(preset.name);
            if (ui.name_char_pos < len)
            {
                ui.name_char_pos++;
                if (ui.name_char_pos >= len)
                {
                    preset.name[ui.name_char_pos] = ' ';
                    preset.name[ui.name_char_pos + 1] = '\0';
                }
            }
        }
        if (input.a.justPressed || input.start.justPressed)
        {
            int len = strlen(preset.name);
            while (len > 0 && preset.name[len - 1] == ' ')
                preset.name[--len] = '\0';
            if (len == 0)
                strncpy(preset.name, "Preset", MAX_PRESET_NAME);
            ui.editing_preset_name = false;
        }
        if (input.b.justPressed)
            ui.editing_preset_name = false;
        return;
    }

    // Field editing
    if (ui.editor_field != FIELD_NONE)
    {
        TimerStep &step = preset.steps[ui.editor_cursor];

        if (input.left.justPressed)
        {
            switch (ui.editor_field)
            {
            case FIELD_LABEL:
            {
                int idx = findLabelIndex(step.label);
                idx = (idx - 1 + STEP_LABEL_COUNT) % STEP_LABEL_COUNT;
                strncpy(step.label, STEP_LABELS[idx], MAX_LABEL_LEN);
                step.label[MAX_LABEL_LEN] = '\0';
                break;
            }
            case FIELD_MINUTES:
            {
                int m = step.duration_sec / 60, s = step.duration_sec % 60;
                m = m > 0 ? m - 1 : 99;
                step.duration_sec = m * 60 + s;
                if (step.duration_sec < MIN_DURATION_SEC)
                    step.duration_sec = MIN_DURATION_SEC;
                if (step.duration_sec > MAX_DURATION_SEC)
                    step.duration_sec = MAX_DURATION_SEC;
                break;
            }
            case FIELD_SECONDS:
            {
                int m = step.duration_sec / 60, s = step.duration_sec % 60;
                s = s > 0 ? s - 1 : 59;
                step.duration_sec = m * 60 + s;
                if (step.duration_sec < MIN_DURATION_SEC)
                    step.duration_sec = MIN_DURATION_SEC;
                break;
            }
            case FIELD_SOUND:
                step.end_sound_enabled = !step.end_sound_enabled;
                break;
            default:
                break;
            }
        }
        if (input.right.justPressed)
        {
            switch (ui.editor_field)
            {
            case FIELD_LABEL:
            {
                int idx = findLabelIndex(step.label);
                idx = (idx + 1) % STEP_LABEL_COUNT;
                strncpy(step.label, STEP_LABELS[idx], MAX_LABEL_LEN);
                step.label[MAX_LABEL_LEN] = '\0';
                break;
            }
            case FIELD_MINUTES:
            {
                int m = step.duration_sec / 60, s = step.duration_sec % 60;
                m = m < 99 ? m + 1 : 0;
                step.duration_sec = m * 60 + s;
                if (step.duration_sec < MIN_DURATION_SEC)
                    step.duration_sec = MIN_DURATION_SEC;
                if (step.duration_sec > MAX_DURATION_SEC)
                    step.duration_sec = MAX_DURATION_SEC;
                break;
            }
            case FIELD_SECONDS:
            {
                int m = step.duration_sec / 60, s = step.duration_sec % 60;
                s = s < 59 ? s + 1 : 0;
                step.duration_sec = m * 60 + s;
                if (step.duration_sec < MIN_DURATION_SEC)
                    step.duration_sec = MIN_DURATION_SEC;
                break;
            }
            case FIELD_SOUND:
                step.end_sound_enabled = !step.end_sound_enabled;
                break;
            default:
                break;
            }
        }
        if (input.a.justPressed)
        {
            switch (ui.editor_field)
            {
            case FIELD_LABEL:
                ui.editor_field = FIELD_MINUTES;
                break;
            case FIELD_MINUTES:
                ui.editor_field = FIELD_SECONDS;
                break;
            case FIELD_SECONDS:
                ui.editor_field = FIELD_SOUND;
                break;
            case FIELD_SOUND:
                ui.editor_field = FIELD_NONE;
                break;
            default:
                break;
            }
        }
        if (input.b.justPressed)
            ui.editor_field = FIELD_NONE;
        return;
    }

    // Navigation
    if (input.up.justPressed && preset.step_count > 0)
    {
        ui.editor_cursor = (ui.editor_cursor - 1 + preset.step_count) % preset.step_count;
    }
    if (input.down.justPressed && preset.step_count > 0)
    {
        ui.editor_cursor = (ui.editor_cursor + 1) % preset.step_count;
    }
    if (input.a.justPressed && preset.step_count > 0)
    {
        ui.editor_field = FIELD_LABEL;
    }
    if (input.start.justPressed)
    {
        ui.editing_preset_name = true;
        ui.name_char_pos = 0;
    }
    if (input.c.justPressed && preset.step_count < MAX_STEPS)
    {
        TimerStep &ns = preset.steps[preset.step_count];
        strncpy(ns.label, "DEV", MAX_LABEL_LEN);
        ns.label[MAX_LABEL_LEN] = '\0';
        ns.duration_sec = 60;
        ns.end_sound_enabled = true;
        preset.step_count++;
        ui.editor_cursor = preset.step_count - 1;
    }
    if (input.d.justPressed && preset.step_count > 0)
    {
        for (int i = ui.editor_cursor; i < preset.step_count - 1; i++)
        {
            preset.steps[i] = preset.steps[i + 1];
        }
        preset.step_count--;
        if (ui.editor_cursor >= preset.step_count && ui.editor_cursor > 0)
        {
            ui.editor_cursor--;
        }
    }
    if (input.b.justPressed)
    {
        ui.screen = SCREEN_MENU;
    }
}

// =====================
//    TIMER RUNNING
// =====================
void uiDrawTimer(lilka::Canvas &c, UIState &ui, uint8_t bright)
{
    c.fillScreen(COLOR_BG);
    TimerEngine &eng = ui.timer_engine;

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    bool uk = ui.lang_uk;

    if (eng.state == TIMER_DONE)
    {
        c.setFont(FONT_10x20);
        c.setTextSize(2);
        c.setTextColor(fg);
        c.drawTextAligned(L(uk, "ALL DONE", "ГОТОВО!"), SCREEN_W / 2, 85, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);
        c.setTextColor(dim);
        char doneBuf[32];
        snprintf(doneBuf, sizeof(doneBuf), "%s %s", btnB(ui.swap_ab), L(uk, "Back to menu", "До меню"));
        c.drawTextAligned(doneBuf, SCREEN_W / 2, 155, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        return;
    }

    TimerStep &step = eng.preset->steps[eng.current_step];

    // Step label + progress
    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    c.setCursor(20, 30);
    c.print(step.label);

    char progBuf[10];
    snprintf(progBuf, sizeof(progBuf), "%d / %d", eng.current_step + 1, eng.preset->step_count);
    c.setCursor(SCREEN_W - 85, 30);
    c.print(progBuf);

    // State
    c.setFont(FONT_9x15);
    c.setTextColor(dim);
    c.setCursor(20, 50);
    switch (eng.state)
    {
    case TIMER_READY:
        c.print(L(uk, "READY", "ГОТОВИЙ"));
        break;
    case TIMER_RUNNING:
        c.print(L(uk, "RUNNING", "ПРАЦЮЄ"));
        break;
    case TIMER_PAUSED:
        c.print(L(uk, "PAUSED", "НА ПАУЗІ"));
        break;
    default:
        break;
    }

    // Large countdown
    char timeBuf[8];
    formatTime(eng.remaining_sec, timeBuf, sizeof(timeBuf));
    c.setFont(FONT_10x20);
    c.setTextSize(3);
    c.setTextColor(fg);
    c.drawTextAligned(timeBuf, SCREEN_W / 2, 95, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
    c.setTextSize(1);

    // Progress bar (larger)
    uint16_t total = step.duration_sec;
    uint16_t elapsed = total - eng.remaining_sec;
    int barW = SCREEN_W - 60;
    int barX = 30;
    int barY = 148;
    int barH = 14;
    c.drawRect(barX, barY, barW, barH, dim);
    if (total > 0)
    {
        int fillW = (int)((long)elapsed * (barW - 2) / total);
        if (fillW > 0)
        {
            c.fillRect(barX + 1, barY + 1, fillW, barH - 2, fg);
        }
    }

    // Next step preview
    c.setFont(FONT_9x15);
    c.setTextColor(dim);
    if (eng.current_step + 1 < eng.preset->step_count)
    {
        TimerStep &next = eng.preset->steps[eng.current_step + 1];
        char nextTime[8];
        formatTime(next.duration_sec, nextTime, sizeof(nextTime));
        c.setCursor(20, 182);
        c.print(L(uk, "Next: ", "Далі: "));
        c.print(next.label);
        c.print("  ");
        c.print(nextTime);
    }
    else
    {
        c.setCursor(20, 182);
        c.print(L(uk, "last step", "Кінець"));
    }

    // Hints
    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 18);
    char hintBuf[48];
    switch (eng.state)
    {
    case TIMER_READY:
        snprintf(hintBuf, sizeof(hintBuf), "%s %s  %s %s",
                 btnA(ui.swap_ab), L(uk, "Start", "Старт"),
                 btnB(ui.swap_ab), L(uk, "Back", "Назад"));
        c.print(hintBuf);
        break;
    case TIMER_RUNNING:
        snprintf(hintBuf, sizeof(hintBuf), "%s %s  [START] %s",
                 btnB(ui.swap_ab), L(uk, "Pause", "Пауза"),
                 L(uk, "Skip", "Пропуск"));
        c.print(hintBuf);
        break;
    case TIMER_PAUSED:
        snprintf(hintBuf, sizeof(hintBuf), "%s %s  %s %s",
                 btnA(ui.swap_ab), L(uk, "Resume", "Далі"),
                 btnB(ui.swap_ab), L(uk, "Back", "Назад"));
        c.print(hintBuf);
        break;
    default:
        break;
    }
}

void uiUpdateTimer(UIState &ui, lilka::State &input)
{
    TimerEngine &eng = ui.timer_engine;
    timerUpdate(eng);

    switch (eng.state)
    {
    case TIMER_READY:
        if (input.a.justPressed)
            timerStart(eng);
        if (input.b.justPressed)
            ui.screen = SCREEN_MENU;
        break;
    case TIMER_RUNNING:
        if (input.b.justPressed)
            timerPause(eng);
        if (input.start.justPressed)
            timerSkipStep(eng);
        break;
    case TIMER_PAUSED:
        if (input.a.justPressed)
            timerResume(eng);
        if (input.b.justPressed)
            ui.screen = SCREEN_MENU;
        break;
    case TIMER_DONE:
        if (input.b.justPressed)
            ui.screen = SCREEN_MENU;
        if (input.a.justPressed)
            timerReset(eng);
        break;
    }
}

// =====================
//      SETTINGS
// =====================
static const char *VOLUME_NAMES[] = {"OFF", "LOW", "MED", "HIGH"};
static const char *VOLUME_NAMES_UK[] = {"ВИМК", "ТИХО", "СЕРЕДНЬО", "ГУЧНО"};
#define SETTINGS_COUNT 6

void uiDrawSettings(lilka::Canvas &c, UIState &ui, AppSettings &settings, uint8_t bright)
{
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    uint16_t hl = colorHL(bright);
    bool uk = ui.lang_uk;

    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    c.drawTextAligned(L(uk, "Settings", "Налаштування"), SCREEN_W / 2, 14, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    c.setFont(FONT_9x15);
    int16_t y0 = 42;
    int16_t rowH = 27;

    // 0: Brightness
    int16_t y = y0;
    if (ui.settings_cursor == 0)
        c.fillRect(15, y - 3, SCREEN_W - 30, 23, hl);
    c.setTextColor(ui.settings_cursor == 0 ? fg : dim);
    c.setCursor(25, y + 12);
    c.print(L(uk, "Brightness ", "Яскравість "));
    for (int b = 0; b < BRIGHTNESS_COUNT; b++)
    {
        c.print(b <= settings.brightness ? "#" : "-");
    }

    // 1: Volume
    y = y0 + rowH;
    if (ui.settings_cursor == 1)
        c.fillRect(15, y - 3, SCREEN_W - 30, 23, hl);
    c.setTextColor(ui.settings_cursor == 1 ? fg : dim);
    c.setCursor(25, y + 12);
    c.print(L(uk, "Volume     ", "Гучність   "));
    c.print(uk ? VOLUME_NAMES_UK[settings.volume] : VOLUME_NAMES[settings.volume]);

    // 2: Language
    y = y0 + rowH * 2;
    if (ui.settings_cursor == 2)
        c.fillRect(15, y - 3, SCREEN_W - 30, 23, hl);
    c.setTextColor(ui.settings_cursor == 2 ? fg : dim);
    c.setCursor(25, y + 12);
    c.print(L(uk, "Language   ", "Мова       "));
    c.print(settings.lang_uk ? "UK" : "EN");

    // 3: Swap A/B
    y = y0 + rowH * 3;
    if (ui.settings_cursor == 3)
        c.fillRect(15, y - 3, SCREEN_W - 30, 23, hl);
    c.setTextColor(ui.settings_cursor == 3 ? fg : dim);
    c.setCursor(25, y + 12);
    c.print(L(uk, "Swap A/B   ", "Заміна A/B "));
    c.print(settings.swap_ab ? L(uk, "ON", "ВКЛ") : L(uk, "OFF", "ВИМК"));

    // 4: External button pin
    y = y0 + rowH * 4;
    if (ui.settings_cursor == 4)
        c.fillRect(15, y - 3, SCREEN_W - 30, 23, hl);
    c.setTextColor(ui.settings_cursor == 4 ? fg : dim);
    c.setCursor(25, y + 12);
    c.print(L(uk, "Ext Btn    ", "Зовн кн    "));
    if (settings.ext_btn_pin_idx == 0)
    {
        c.print(L(uk, "OFF", "ВИМК"));
    }
    else
    {
        char pinBuf[4];
        snprintf(pinBuf, sizeof(pinBuf), "%d", EXT_BTN_PINS[settings.ext_btn_pin_idx - 1]);
        c.print(pinBuf);
    }

    // 5: WiFi
    y = y0 + rowH * 5;
    if (ui.settings_cursor == 5)
        c.fillRect(15, y - 3, SCREEN_W - 30, 23, hl);
    c.setTextColor(ui.settings_cursor == 5 ? fg : dim);
    c.setCursor(25, y + 12);
    c.print("WiFi       ");
    if (settings.wifi_enabled)
    {
        c.print(L(uk, "ON", "ВКЛ"));
        if (wifiIsRunning())
        {
            c.setFont(FONT_8x13);
            c.print(" ");
            c.print(wifiGetIP().c_str());
            c.setFont(FONT_9x15);
        }
    }
    else
    {
        c.print(L(uk, "OFF", "ВИМК"));
    }

    // Hints
    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 18);
    char hBuf[40];
    snprintf(hBuf, sizeof(hBuf), "[L/R] %s  %s %s",
             L(uk, "Adjust", "Змінити"), btnB(ui.swap_ab), L(uk, "Back", "Назад"));
    c.print(hBuf);
}

void uiUpdateSettings(UIState &ui, lilka::State &input, AppSettings &settings)
{
    if (input.up.justPressed)
        ui.settings_cursor = (ui.settings_cursor - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
    if (input.down.justPressed)
        ui.settings_cursor = (ui.settings_cursor + 1) % SETTINGS_COUNT;

    if (input.left.justPressed || input.right.justPressed)
    {
        switch (ui.settings_cursor)
        {
        case 0: // brightness
            if (input.right.justPressed && settings.brightness < BRIGHTNESS_COUNT - 1)
                settings.brightness++;
            if (input.left.justPressed && settings.brightness > 0)
                settings.brightness--;
            break;
        case 1: // volume
            if (input.right.justPressed && settings.volume < VOLUME_COUNT - 1)
                settings.volume++;
            if (input.left.justPressed && settings.volume > 0)
                settings.volume--;
            break;
        case 2: // language
            settings.lang_uk = !settings.lang_uk;
            ui.lang_uk = settings.lang_uk;
            break;
        case 3: // swap A/B
            settings.swap_ab = !settings.swap_ab;
            ui.swap_ab = settings.swap_ab;
            break;
        case 4: // external button pin
            if (input.right.justPressed)
                settings.ext_btn_pin_idx = (settings.ext_btn_pin_idx + 1) % (EXT_BTN_PIN_COUNT + 1);
            if (input.left.justPressed)
                settings.ext_btn_pin_idx = (settings.ext_btn_pin_idx + EXT_BTN_PIN_COUNT) % (EXT_BTN_PIN_COUNT + 1);
            break;
        case 5: // WiFi
            settings.wifi_enabled = !settings.wifi_enabled;
            break;
        }
    }
    if (input.b.justPressed)
        ui.screen = SCREEN_MENU;
}

// =====================
//    DMAX TIMER
// =====================

void uiDrawDmaxTimer(lilka::Canvas &c, UIState &ui, uint8_t bright)
{
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    bool uk = ui.lang_uk;

    // Title
    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    c.drawTextAligned("Dmax Test", SCREEN_W / 2, 18, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    switch (ui.dmax_state)
    {
    case DMAX_READY:
    {
        c.setFont(FONT_9x15);
        c.setTextColor(dim);
        c.setCursor(20, 70);
        char dbuf[24];
        snprintf(dbuf, sizeof(dbuf), "Delay: %ds", ui.dmax_delay);
        c.drawTextAligned(dbuf, SCREEN_W / 2, 80, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

        c.setFont(FONT_10x20);
        c.setTextSize(3);
        c.setTextColor(fg);
        c.drawTextAligned("--:--", SCREEN_W / 2, 130, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);

        c.setFont(FONT_8x13);
        c.setTextColor(dim);
        c.setCursor(20, SCREEN_H - 18);
        char hBuf[40];
        snprintf(hBuf, sizeof(hBuf), "%s %s  %s %s",
                 btnA(ui.swap_ab), L(uk, "Start", "Старт"),
                 btnB(ui.swap_ab), L(uk, "Back", "Назад"));
        c.print(hBuf);
        break;
    }
    case DMAX_DELAY:
    {
        c.setFont(FONT_9x15);
        c.setTextColor(dim);
        c.drawTextAligned(L(uk, "DELAY", "ЗАТРИМКА"), SCREEN_W / 2, 50, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

        c.setFont(FONT_10x20);
        c.setTextSize(3);
        c.setTextColor(fg);
        char tBuf[8];
        snprintf(tBuf, sizeof(tBuf), "%d", ui.dmax_remaining);
        c.drawTextAligned(tBuf, SCREEN_W / 2, 110, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);

        c.setFont(FONT_9x15);
        c.setTextColor(dim);
        c.drawTextAligned(L(uk, "Dip test strip", "Занур клаптик"), SCREEN_W / 2, 170, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        break;
    }
    case DMAX_SIGNAL:
    {
        c.setFont(FONT_10x20);
        c.setTextSize(2);
        c.setTextColor(fg);
        c.drawTextAligned(L(uk, "SUBMERGE!", "ЗАНУРЮЙ!"), SCREEN_W / 2, 110, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);
        break;
    }
    case DMAX_TIMING:
    {
        c.setFont(FONT_9x15);
        c.setTextColor(dim);
        c.drawTextAligned(L(uk, "TIMING", "ВІДЛІК"), SCREEN_W / 2, 50, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

        unsigned long elapsed = millis() - ui.dmax_start_ms;
        uint16_t sec = elapsed / 1000;
        c.setFont(FONT_10x20);
        c.setTextSize(3);
        c.setTextColor(fg);
        char tBuf[8];
        snprintf(tBuf, sizeof(tBuf), "%02d:%02d", sec / 60, sec % 60);
        c.drawTextAligned(tBuf, SCREEN_W / 2, 110, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);

        c.setFont(FONT_9x15);
        c.setTextColor(dim);
        c.drawTextAligned(L(uk, "Watch for even Dmax", "Чекай рівний Dmax"), SCREEN_W / 2, 170, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

        c.setFont(FONT_8x13);
        c.setTextColor(dim);
        c.setCursor(20, SCREEN_H - 18);
        char hBuf[32];
        snprintf(hBuf, sizeof(hBuf), "%s %s", btnA(ui.swap_ab), L(uk, "Stop", "Стоп"));
        c.print(hBuf);
        break;
    }
    case DMAX_DONE:
    {
        c.setFont(FONT_9x15);
        c.setTextColor(dim);
        c.drawTextAligned(L(uk, "DEV TIME", "ЧАС ПРОЯВКИ"), SCREEN_W / 2, 50, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

        uint16_t sec = ui.dmax_result_ms / 1000;
        c.setFont(FONT_10x20);
        c.setTextSize(3);
        c.setTextColor(fg);
        char tBuf[8];
        snprintf(tBuf, sizeof(tBuf), "%02d:%02d", sec / 60, sec % 60);
        c.drawTextAligned(tBuf, SCREEN_W / 2, 110, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);
        c.setTextSize(1);

        c.setFont(FONT_8x13);
        c.setTextColor(dim);
        c.setCursor(20, SCREEN_H - 18);
        char hBuf[40];
        snprintf(hBuf, sizeof(hBuf), "%s %s  %s %s",
                 btnA(ui.swap_ab), L(uk, "Again", "Знову"),
                 btnB(ui.swap_ab), L(uk, "Back", "Назад"));
        c.print(hBuf);
        break;
    }
    }
}

void uiUpdateDmaxTimer(UIState &ui, lilka::State &input, uint8_t volume)
{
    switch (ui.dmax_state)
    {
    case DMAX_READY:
        if (input.a.justPressed)
        {
            ui.dmax_remaining = ui.dmax_delay;
            ui.dmax_last_beeped = 0xFFFF;
            ui.dmax_tick_ms = millis();
            ui.dmax_state = DMAX_DELAY;
            timerBeep(BEEP_START_FREQ, BEEP_START_DUR, volume);
        }
        if (input.b.justPressed)
            ui.screen = SCREEN_MENU;
        break;

    case DMAX_DELAY:
    {
        unsigned long now = millis();
        if (now - ui.dmax_tick_ms >= 1000)
        {
            uint16_t passed = (now - ui.dmax_tick_ms) / 1000;
            ui.dmax_tick_ms += passed * 1000;
            if (ui.dmax_remaining <= passed)
            {
                ui.dmax_remaining = 0;
            }
            else
            {
                ui.dmax_remaining -= passed;
            }
        }

        if (ui.dmax_remaining == 0)
        {
            timerTripleBeep(BEEP_DONE_FREQ, BEEP_DONE_DUR, volume);
            ui.dmax_signal_ms = millis();
            ui.dmax_state = DMAX_SIGNAL;
        }

        if (input.b.justPressed)
        {
            ui.dmax_state = DMAX_READY;
        }
        break;
    }

    case DMAX_SIGNAL:
        if (millis() - ui.dmax_signal_ms >= 1000)
        {
            ui.dmax_start_ms = millis();
            ui.dmax_state = DMAX_TIMING;
        }
        break;

    case DMAX_TIMING:
        if (input.a.justPressed)
        {
            ui.dmax_result_ms = millis() - ui.dmax_start_ms;
            timerBeep(BEEP_START_FREQ, BEEP_START_DUR, volume);
            ui.dmax_state = DMAX_DONE;
        }
        if (input.b.justPressed)
        {
            ui.dmax_state = DMAX_READY;
        }
        break;

    case DMAX_DONE:
        if (input.a.justPressed)
        {
            ui.dmax_state = DMAX_READY;
        }
        if (input.b.justPressed)
        {
            ui.screen = SCREEN_MENU;
        }
        break;
    }
}

// =====================
//    DMAX EDITOR
// =====================

void uiDrawDmaxEditor(lilka::Canvas &c, UIState &ui, TimerPreset &preset, uint8_t bright)
{
    c.fillScreen(COLOR_BG);

    uint16_t fg = colorFG(bright);
    uint16_t dim = colorDim(bright);
    bool uk = ui.lang_uk;

    c.setFont(FONT_10x20);
    c.setTextSize(1);
    c.setTextColor(fg);
    c.drawTextAligned("Dmax Test", SCREEN_W / 2, 18, lilka::ALIGN_CENTER, lilka::ALIGN_START);

    c.setFont(FONT_10x20);
    c.setTextColor(fg);
    char dBuf[24];
    snprintf(dBuf, sizeof(dBuf), "Delay: %ds", preset.steps[0].duration_sec);
    c.drawTextAligned(dBuf, SCREEN_W / 2, 100, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

    c.setFont(FONT_9x15);
    c.setTextColor(dim);
    c.drawTextAligned(L(uk, "Time before submerge", "Час до повного занурення"), SCREEN_W / 2, 140, lilka::ALIGN_CENTER, lilka::ALIGN_CENTER);

    c.setFont(FONT_8x13);
    c.setTextColor(dim);
    c.setCursor(20, SCREEN_H - 18);
    char hBuf[40];
    snprintf(hBuf, sizeof(hBuf), "[L/R] %s  %s %s",
             L(uk, "Adjust", "Змінити"), btnB(ui.swap_ab), L(uk, "Back", "Назад"));
    c.print(hBuf);
}

void uiUpdateDmaxEditor(UIState &ui, lilka::State &input, TimerPreset &preset)
{
    uint16_t &delay = preset.steps[0].duration_sec;
    if (input.right.justPressed && delay < 60)
        delay++;
    if (input.left.justPressed && delay > 1)
        delay--;
    if (input.b.justPressed)
        ui.screen = SCREEN_MENU;
}
