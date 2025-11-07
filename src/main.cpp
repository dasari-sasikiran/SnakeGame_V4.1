#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "game.h"
#include "ir_control.h"
#include "web_control.h"
#include "buzzer.h"
#include "voice.h"

unsigned long lastMove = 0;

void setup()
{
  Serial.begin(115200);
  delay(100);

  initDisplay();
  initFS();
  initWiFi();
  initWebSocket();
  initWebServer();

  pinMode(BUZZER_PIN, OUTPUT);

  initIR();

  initVoice(); // initialize EI runner and voice subsystem

  restart_with_splash();
}

void loop()
{
  wsCleanupClients();
  handleIRInput(); // IR remote check

  // run on-device voice inference (Edge Impulse)
  voiceLoop();

  if (!paused && !game_over)
  {
    unsigned long now = millis();
    if (now - lastMove >= (unsigned long)snake_speed)
    {
      lastMove = now;
      move_snake();
      if (food_eaten)
        safeSpawnFood();
      draw_frame();
    }
  }
  else
  {
    static unsigned long lastRedraw = 0;
    if (millis() - lastRedraw > 200)
    {
      if (game_over)
        draw_game_over_screen();
      else
        draw_frame();
      lastRedraw = millis();
    }
  }

  // handle actions requested by web socket (run in main context)
  if (ws_setDirection >= 0)
  {
    direction = ws_setDirection;
    ws_setDirection = -1;
  }

  if (ws_togglePause)
  {
    ws_togglePause = false;
    paused = !paused;
  }

  if (ws_toggleSound)
  {
    ws_toggleSound = false;
    sound_enabled = !sound_enabled;
  }

  if (ws_needBeep)
  {
    ws_needBeep = false;
    playClickBeep();
  }

  if (ws_needRedraw)
  {
    ws_needRedraw = false;
    draw_frame();
  }

  if (ws_needRestart)
  {
    ws_needRestart = false;
    restart_with_splash();
  }

  // WINDOW: if the web client sent a transcript via "VOICE:<transcript>" then
  // main loop picks it up and calls the text->action mapping function.
  if (ws_voiceCommand)
  {
    // clear flag first to avoid races
    ws_voiceCommand = false;
    handleVoiceCommand(ws_voiceTranscript);
    ws_voiceTranscript = "";
  }

  delay(10);
}
