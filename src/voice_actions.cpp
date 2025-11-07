// voice_actions.cpp
// Re-introduces handleVoiceCommand(const String&) so linker resolves references.
// Maps EI labels / transcripts to your existing websocket/game flags.

#include <Arduino.h>
#include "web_control.h" // provides extern flags and notifyClients()
#include "buzzer.h"      // optional: playClickBeep()

// forward declaration for playClickBeep() if buzzer.h doesn't provide it
#ifndef playClickBeep
extern void playClickBeep();
#endif

// Normalize text: trim + upper-case
static String normalizeTranscript(const String &in) {
  String s = in;
  s.trim();
  s.toUpperCase();
  return s;
}

// The function required by main.cpp and voice.cpp
void handleVoiceCommand(const String &transcript) {
  String t = normalizeTranscript(transcript);
  if (t.length() == 0) {
    Serial.println(F("[Voice] handleVoiceCommand called with empty transcript"));
    return;
  }

  Serial.print(F("[Voice] Processing transcript: "));
  Serial.println(t);

  bool didAction = false;

  // Map known labels (support both model labels and generic phrases)
  // Prefer direct label names from your list, but also accept words/phrases.

  // Move up
  if (t.indexOf("MOVE_UP") >= 0 || t.indexOf("MOVE UP") >= 0 ||
      t.indexOf("MOVEUP") >= 0 || t.indexOf("MOVEUP") >= 0 ||
      t.indexOf("MOVEUP") >= 0 || t.indexOf("UP") >= 0 ||
      t.indexOf("MOVEUP") >= 0 || t.indexOf("UPSIDE") >= 0) {
    ws_setDirection = 0; // up
    didAction = true;
  }
  // Move right
  else if (t.indexOf("MOVE_RIGHT") >= 0 || t.indexOf("MOVE RIGHT") >= 0 ||
           t.indexOf("MOVERIGHT") >= 0 || t.indexOf("RIGHT") >= 0) {
    ws_setDirection = 1; // right
    didAction = true;
  }
  // Move down
  else if (t.indexOf("MOVE_DOWN") >= 0 || t.indexOf("MOVE DOWN") >= 0 ||
           t.indexOf("MOVEDOWN") >= 0 || t.indexOf("DOWN") >= 0 ||
           t.indexOf("DOWNWARDS") >= 0) {
    ws_setDirection = 2; // down
    didAction = true;
  }
  // Move left
  else if (t.indexOf("MOVE_LEFT") >= 0 || t.indexOf("MOVE LEFT") >= 0 ||
           t.indexOf("MOVELEFT") >= 0 || t.indexOf("LEFT") >= 0) {
    ws_setDirection = 3; // left
    didAction = true;
  }
  // Pause / play (toggle)
  else if (t.indexOf("PAUSE") >= 0 || t.indexOf("PAUSE_GAME") >= 0 || t.indexOf("PLAY_GAME") >= 0 || t.indexOf("PLAY") >= 0) {
    ws_togglePause = true;
    didAction = true;
  }
  // Restart
  else if (t.indexOf("RESTART") >= 0 || t.indexOf("RESTART_GAME") >= 0 || t.indexOf("START OVER") >= 0) {
    ws_needRestart = true;
    didAction = true;
  }
  // Mute / unmute
  else if (t.indexOf("MUTE") >= 0 || t.indexOf("MUTE_SOUND") >= 0 || t.indexOf("MUTE_SOUND") >= 0) {
    ws_toggleSound = true;
    didAction = true;
  }
  else if (t.indexOf("UNMUTE") >= 0 || t.indexOf("SOUND ON") >= 0 || t.indexOf("UNMUTE_SOUND") >= 0) {
    ws_toggleSound = true;
    didAction = true;
  }
  // special short labels that may be noise indicators — don't trigger action
  else if (t == "NOISE" || t == "NOICE" || t == "SILENCE") {
    // ignore or log
    Serial.println(F("[Voice] Detected noise/silence label — no action taken"));
  }
  else {
    // Not recognized — log it
    Serial.print(F("[Voice] Unknown transcript/label: "));
    Serial.println(t);
  }

  // If action chosen, request beep & redraw for UI feedback
  if (didAction) {
    ws_needBeep = true;
    ws_needRedraw = true;
  }

  // Play feedback sound if available (optional)
  #ifdef playClickBeep
  playClickBeep();
  #else
  // If buzzer available under different name, call that instead
  #endif

  // Inform any connected web clients (console) about the recognized phrase.
  String outMsg = "VOICE_RX:" + t;
  notifyClients(outMsg);

  Serial.print(F("[Voice] Action processed (didAction="));
  Serial.print(didAction ? "true" : "false");
  Serial.println(F(")"));
}
