#include "ir_control.h"
#include "config.h"
#include "game.h"
#include "display.h"
#include "buzzer.h"

#include <IRremote.hpp>   // IRremote v4.x, provides IrReceiver

// Initialize IR receiver (call this from setup)
void initIR()
{
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK); // initialize IR receiver with feedback LED on
  Serial.println("IR receiver initialized");
}

// Call this regularly from loop()
void handleIRInput()
{
  if (IrReceiver.decode()) {
    uint32_t irCode = IrReceiver.decodedIRData.decodedRawData;
    Serial.print("IR Code: 0x"); Serial.println(irCode, HEX);

    if (irCode == IR_UP_CODE && direction != 2) {
      direction = 0;
      playClickBeep();
    }
    else if (irCode == IR_DOWN_CODE && direction != 0) {
      direction = 2;
      playClickBeep();
    }
    else if (irCode == IR_LEFT_CODE && direction != 1) {
      direction = 3;
      playClickBeep();
    }
    else if (irCode == IR_RIGHT_CODE && direction != 3) {
      direction = 1;
      playClickBeep();
    }
    else if (irCode == IR_PAUSE_CODE) {
      paused = !paused;
      playClickBeep();
      draw_frame();
    }
    else if (irCode == IR_RESET_CODE) {
      restart_with_splash();
    }
    else if (irCode == IR_SOUND_TOGGLE_CODE) {
      sound_enabled = !sound_enabled;
      draw_frame();
      if (sound_enabled) playClickBeep();
    }

    IrReceiver.resume(); // ready for next code
  }
}
