#pragma once

#include "storage.h"

enum TimerState {
    TIMER_READY,
    TIMER_RUNNING,
    TIMER_PAUSED,
    TIMER_DONE
};

struct TimerEngine {
    TimerPreset* preset;
    uint8_t current_step;
    uint16_t remaining_sec;
    unsigned long last_tick_ms;
    TimerState state;
    uint8_t volume;

    // Sound tracking
    bool beeped_start;
    uint16_t last_beeped_30s;
    uint16_t last_beeped_sec;

    // Done state
    unsigned long done_time_ms; // when TIMER_DONE was entered
};

void timerInit(TimerEngine& engine, TimerPreset* preset, uint8_t volume);
void timerStart(TimerEngine& engine);
void timerPause(TimerEngine& engine);
void timerResume(TimerEngine& engine);
void timerSkipStep(TimerEngine& engine);
void timerReset(TimerEngine& engine);  // reset to READY for same preset
void timerUpdate(TimerEngine& engine); // call every loop iteration
void timerBeep(uint16_t freq, uint16_t base_duration_ms, uint8_t volume);
void timerDoubleBeep(uint16_t freq, uint16_t base_duration_ms, uint8_t volume);
void timerTripleBeep(uint16_t freq, uint16_t base_duration_ms, uint8_t volume);
