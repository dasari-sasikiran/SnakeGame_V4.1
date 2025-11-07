#ifndef VOICE_H
#define VOICE_H

#include <Arduino.h>

void initVoice();
void voiceLoop();
void handleVoiceCommand(const String &transcript);

// Called by web_control when it receives binary audio frames
// samples: pointer to int16_t PCM samples (little-endian), count = number of samples
void microphone_feed(const int16_t *samples, size_t count);

#endif // VOICE_H
