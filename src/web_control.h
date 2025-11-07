#ifndef WEB_CONTROL_H
#define WEB_CONTROL_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// websocket flags (kept names)
extern volatile bool ws_needBeep;
extern volatile bool ws_needRedraw;
extern volatile bool ws_needRestart;
extern volatile int ws_setDirection; // -1 = no change, 0..3
extern volatile bool ws_togglePause;
extern volatile bool ws_toggleSound;

// voice: browser -> ESP signaling (set when web client sends "VOICE:<transcript>")
extern volatile bool ws_voiceCommand;      // set true by websocket when transcript arrives
extern String ws_voiceTranscript;          // transcript content (not volatile; updated by ws handler)

// helper functions provided by web_control.cpp
void initFS();
void initWiFi();
void initWebSocket();
void initWebServer();

void wsCleanupClients();

void notifyClients(const String &msg);

#endif // WEB_CONTROL_H
