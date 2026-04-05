#pragma once

#include <lilka.h>

// --- Limits ---
#define MAX_STEPS 10
#define MAX_LABEL_LEN 5
#define MAX_PRESETS 4
#define MAX_PRESET_NAME 10

// --- Colors ---
// Darkroom safe: red on black only.
// Software brightness: scale red channel intensity.
// Brightness level 0..4 maps to red values below.
const uint8_t BRIGHTNESS_RED[] = {40, 80, 140, 200, 255};
#define BRIGHTNESS_COUNT 5
#define DEFAULT_BRIGHTNESS 2

// Color helpers — call with current brightness index
inline uint16_t colorFG(uint8_t bright)
{
    return lilka::display.color565(BRIGHTNESS_RED[bright], 0, 0);
}
inline uint16_t colorDim(uint8_t bright)
{
    return lilka::display.color565(BRIGHTNESS_RED[bright] / 3, 0, 0);
}
inline uint16_t colorHL(uint8_t bright)
{
    return lilka::display.color565(BRIGHTNESS_RED[bright] / 7, 0, 0);
}
#define COLOR_BG 0x0000

// --- Volume ---
#define VOLUME_OFF 0
#define VOLUME_LOW 1
#define VOLUME_MED 2
#define VOLUME_HIGH 3
#define VOLUME_COUNT 4
#define DEFAULT_VOLUME VOLUME_MED

const float VOLUME_MULT[] = {0.0f, 0.5f, 1.0f, 1.5f};

// --- Sound frequencies & durations ---
#define BEEP_START_FREQ 880
#define BEEP_START_DUR 100

#define BEEP_30S_FREQ 660
#define BEEP_30S_DUR 50

#define BEEP_SEC_FREQ 880
#define BEEP_SEC_DUR 80

#define BEEP_3S_FREQ 1100
#define BEEP_3S_DUR 80

#define BEEP_END_FREQ 880
#define BEEP_END_DUR 150

#define BEEP_DONE_FREQ 1100
#define BEEP_DONE_DUR 200

// --- Predefined step labels ---
const char *const STEP_LABELS[] = {
    "DEV", "STOP", "FIX", "STB", "WASH", "HCA", "#1", "#2", "#3", "#4", "#5"};
#define STEP_LABEL_COUNT 10

// --- Timer limits ---
#define MIN_DURATION_SEC 1
#define MAX_DURATION_SEC 5999 // 99:59

// --- Dmax test ---
#define DMAX_PRESET_INDEX 3
#define DMAX_DEFAULT_DELAY 10

// --- WiFi ---
#define WIFI_AP_SSID "Darkroom Timer"
#define WIFI_AP_PASS "darkroom"
#define WIFI_HOSTNAME "timer"

// --- Display ---
#define SCREEN_W 280
#define SCREEN_H 240
