#include "web_control.h"
#include "config.h"
#include "game.h"
#include "display.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

// WiFi credentials (original names, change if needed)
const char *ssid = "ENTER YOU WIFI SSID";
const char *password = "ENTER YOUR WIFI PASSWORD";

// websocket flags (same names)
volatile bool ws_needBeep = false;
volatile bool ws_needRedraw = false;
volatile bool ws_needRestart = false;
volatile int ws_setDirection = -1;
volatile bool ws_togglePause = false;
volatile bool ws_toggleSound = false;

// Voice command support (default values)
volatile bool ws_voiceCommand = false;
String ws_voiceTranscript = "";


extern void microphone_feed(const int16_t *samples, size_t count);



AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// forward decls
static void handleIWebSocketMessage(void *arg, uint8_t *data, size_t len);

void notifyClients(const String &msg) {
  ws.textAll(msg);
}

void wsCleanupClients() {
  ws.cleanupClients();
}

void onEvent(AsyncWebSocket *serverPtr, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("Client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleIWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleIWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (!info) return;

  // We only handle complete frames (no continuation handling here)
  if (!(info->final && info->index == 0 && info->len == len)) return;

  // --- BINARY frames (audio chunks from browser) ---
  if (info->opcode == WS_BINARY) {
    // Expect PCM16 little-endian samples from the browser
    if (len < 2) {
      // nothing useful
      return;
    }

    // len must be multiple of 2 for int16 samples
    if ((len & 1) != 0) {
      Serial.printf("WS: binary len not multiple of 2 (%u)\n", (unsigned)len);
      // We can still try to process floor(len/2) samples
    }

    size_t samples = len / 2;

    // Cast the incoming bytes to int16_t* (browser sent little-endian PCM16)
    // NOTE: data is valid for the duration of this call; microphone_feed should not keep the pointer.
    const int16_t *samples_ptr = (const int16_t *)data;

    // Forward to the voice module which will buffer and run inference (continuous/slide)
    microphone_feed(samples_ptr, samples);

    // We handled binary payload; return (do not process as text)
    return;
  }

  // --- TEXT frames ---
  if (info->opcode == WS_TEXT) {

    const size_t MAX_MSG_LEN = 256;
    if (len == 0 || len > MAX_MSG_LEN) {
      Serial.printf("WS: ignored (len=%u)\n", (unsigned)len);
      return;
    }

    char buf[MAX_MSG_LEN + 1];
    memcpy(buf, data, len);
    buf[len] = '\0';
    String msg = String(buf);
    msg.trim();

    Serial.printf("WS RX: %s\n", msg.c_str());

    // Audio control messages sent by the browser streamer:
    // e.g. "AUDIO_START:sr=16000;ch=1;fmt=pcm16;framesz=1024" or "AUDIO_STOP"
    if (msg.startsWith("AUDIO_START")) {
      Serial.printf("WS: AUDIO_START -> %s\n", msg.c_str());
      // Optional: if you later implement microphone_start(sampleRate, channels, frameSize)
      // you can parse the parameters here and call that function.
      // For now we just log and return.
      return;
    } else if (msg.equals("AUDIO_STOP")) {
      Serial.println("WS: AUDIO_STOP");
      // Optional: call microphone_stop() if implemented.
      return;
    }

    // Existing command mapping (preserved)
    if (msg.equals("UP_HIGH") && direction != 2) ws_setDirection = 0;
    else if (msg.equals("DOWN_HIGH") && direction != 0) ws_setDirection = 2;
    else if (msg.equals("LEFT_HIGH") && direction != 1) ws_setDirection = 3;
    else if (msg.equals("RIGHT_HIGH") && direction != 3) ws_setDirection = 1;
    else if (msg.equals("PAUSE_PLAY")) ws_togglePause = true;
    else if (msg.equals("RESTART")) ws_needRestart = true;
    else if (msg.equals("MUTE")) ws_toggleSound = true;
    // Voice transcript from web client: format "VOICE:<transcript>"
    else if (msg.startsWith("VOICE:")) {
      String transcript = msg.substring(6);
      transcript.trim();
      if (transcript.length() > 0) {
        ws_voiceTranscript = transcript;
        ws_voiceCommand = true; // main loop will pick this up and call handleVoiceCommand()
        Serial.printf("WS: voice transcript queued: %s\n", transcript.c_str());
      }
    }

    if (ws_setDirection >= 0 || ws_togglePause || ws_toggleSound)
    {
      ws_needBeep = true;
      ws_needRedraw = true;
    }

    // done with text frame
    return;
  }

  // Other opcodes (PING/PONG/others) — ignore
}


void initFS()
{
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  } else {
    Serial.println("SPIFFS mounted");
  }
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // optional timeout
    if (millis() - start > 30000) {
      Serial.println("\nWiFi connect timeout. Restarting...");
      ESP.restart();
    }
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initWebServer()
{
  // serve files from SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
  Serial.println("Server started");
}
