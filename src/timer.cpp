#include "timer.h"

void timerBeep(uint16_t freq, uint16_t base_duration_ms, uint8_t volume) {
    if (volume == VOLUME_OFF) return;
    uint16_t dur = (uint16_t)(base_duration_ms * VOLUME_MULT[volume]);
    lilka::buzzer.play(freq, dur);
}

void timerDoubleBeep(uint16_t freq, uint16_t base_duration_ms, uint8_t volume) {
    if (volume == VOLUME_OFF) return;
    uint16_t dur = (uint16_t)(base_duration_ms * VOLUME_MULT[volume]);
    // Play first beep, delay handled by non-blocking buzzer
    // We use a simple melody approach
    lilka::Tone melody[] = {
        {freq, 8},  // short note
        {0, 16},    // pause
        {freq, 8},  // short note
    };
    lilka::buzzer.playMelody(melody, 3, (uint16_t)(60000 / (dur * 4)));
}

void timerTripleBeep(uint16_t freq, uint16_t base_duration_ms, uint8_t volume) {
    if (volume == VOLUME_OFF) return;
    uint16_t dur = (uint16_t)(base_duration_ms * VOLUME_MULT[volume]);
    lilka::Tone melody[] = {
        {freq, 4},
        {0, 8},
        {freq, 4},
        {0, 8},
        {freq, 4},
    };
    lilka::buzzer.playMelody(melody, 5, (uint16_t)(60000 / (dur * 4)));
}

static void startCurrentStep(TimerEngine& engine) {
    if (engine.current_step >= engine.preset->step_count) {
        engine.state = TIMER_DONE;
        engine.done_time_ms = millis();
        timerTripleBeep(BEEP_DONE_FREQ, BEEP_DONE_DUR, engine.volume);
        return;
    }

    TimerStep& step = engine.preset->steps[engine.current_step];
    engine.remaining_sec = step.duration_sec;
    engine.last_tick_ms = millis();
    engine.state = TIMER_RUNNING;
    engine.beeped_start = false;
    engine.last_beeped_30s = 0xFFFF;
    engine.last_beeped_sec = 0xFFFF;

    timerBeep(BEEP_START_FREQ, BEEP_START_DUR, engine.volume);
    engine.beeped_start = true;
}

void timerInit(TimerEngine& engine, TimerPreset* preset, uint8_t volume) {
    engine.preset = preset;
    engine.current_step = 0;
    engine.remaining_sec = preset->steps[0].duration_sec;
    engine.state = TIMER_READY;
    engine.volume = volume;
    engine.beeped_start = false;
    engine.last_beeped_30s = 0xFFFF;
    engine.last_beeped_sec = 0xFFFF;
    engine.last_tick_ms = 0;
    engine.done_time_ms = 0;
}

void timerReset(TimerEngine& engine) {
    timerInit(engine, engine.preset, engine.volume);
}

void timerStart(TimerEngine& engine) {
    if (engine.state != TIMER_READY) return;
    startCurrentStep(engine);
}

void timerPause(TimerEngine& engine) {
    if (engine.state != TIMER_RUNNING) return;
    engine.state = TIMER_PAUSED;
}

void timerResume(TimerEngine& engine) {
    if (engine.state != TIMER_PAUSED) return;
    engine.state = TIMER_RUNNING;
    engine.last_tick_ms = millis();
}

void timerSkipStep(TimerEngine& engine) {
    if (engine.state == TIMER_DONE || engine.state == TIMER_READY) return;

    // End sound for current step if enabled
    TimerStep& step = engine.preset->steps[engine.current_step];
    if (step.end_sound_enabled) {
        timerDoubleBeep(BEEP_END_FREQ, BEEP_END_DUR, engine.volume);
    }

    engine.current_step++;
    startCurrentStep(engine);
}

void timerUpdate(TimerEngine& engine) {
    // Auto-reset to READY after 3 seconds in DONE state
    if (engine.state == TIMER_DONE) {
        if (millis() - engine.done_time_ms >= 3000) {
            timerReset(engine);
        }
        return;
    }

    if (engine.state != TIMER_RUNNING) return;

    unsigned long now = millis();
    unsigned long elapsed = now - engine.last_tick_ms;

    if (elapsed >= 1000) {
        uint16_t secs_passed = elapsed / 1000;
        engine.last_tick_ms += secs_passed * 1000;

        if (engine.remaining_sec <= secs_passed) {
            engine.remaining_sec = 0;
        } else {
            engine.remaining_sec -= secs_passed;
        }
    }

    // Check if step completed
    if (engine.remaining_sec == 0) {
        TimerStep& step = engine.preset->steps[engine.current_step];
        if (step.end_sound_enabled) {
            timerDoubleBeep(BEEP_END_FREQ, BEEP_END_DUR, engine.volume);
        }
        engine.current_step++;
        startCurrentStep(engine); // auto-advance immediately
        return;
    }

    // Sound triggers based on remaining time
    uint16_t rem = engine.remaining_sec;

    // Every 30 seconds mark
    uint16_t mark30 = (rem / 30) * 30;
    if (mark30 > 0 && rem == mark30 && engine.last_beeped_30s != mark30 && rem > 10) {
        timerBeep(BEEP_30S_FREQ, BEEP_30S_DUR, engine.volume);
        engine.last_beeped_30s = mark30;
    }

    // Last 10 seconds: beep every second
    if (rem <= 10 && rem > 3 && engine.last_beeped_sec != rem) {
        timerBeep(BEEP_SEC_FREQ, BEEP_SEC_DUR, engine.volume);
        engine.last_beeped_sec = rem;
    }

    // Last 3 seconds: higher pitch
    if (rem <= 3 && rem > 0 && engine.last_beeped_sec != rem) {
        timerBeep(BEEP_3S_FREQ, BEEP_3S_DUR, engine.volume);
        engine.last_beeped_sec = rem;
    }
}
