#include "buzzer.h"
#include "config.h"

bool sound_enabled = true; // preserved name

void playTone(uint32_t freq, uint32_t dur)
{
  if (!sound_enabled)
    return;
  tone(BUZZER_PIN, freq);
  delay(dur);
  noTone(BUZZER_PIN);
}

void playStartupBeep()
{
  if (sound_enabled)
  {
    playTone(1000, 100);
    delay(60);
    playTone(1200, 100);
  }
}

void playEatBeep()
{
  if (sound_enabled)
    playTone(1500, 100);
}

void playClickBeep()
{
  if (sound_enabled)
    playTone(2000, 30);
}

void playGameOverBeep()
{
  if (!sound_enabled)
    return;
  for (int i = 0; i < 5; i++)
  {
    playTone(600 + i * 120, 40);
    delay(40 + i * 20);
  }
}