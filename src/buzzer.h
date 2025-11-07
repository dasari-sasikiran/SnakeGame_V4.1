#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

extern bool sound_enabled;

void playTone(uint32_t freq, uint32_t dur);
void playStartupBeep();
void playEatBeep();
void playClickBeep();
void playGameOverBeep();

#endif // BUZZER_H