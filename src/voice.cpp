#include "voice.h"
#include "web_control.h" // share ws flags and notifyClients()
#include "buzzer.h"
#include <Arduino.h>


#ifdef EI_PORTING_ARDUINO
  #undef EI_PORTING_ARDUINO
#endif
#define EI_PORTING_ARDUINO 1

#include "snake-voice-console_inferencing.h"

// Buffer to accumulate incoming PCM16 samples from browser stream.
// We'll accumulate up to EI_CLASSIFIER_RAW_SAMPLE_COUNT samples (model window).
static volatile int16_t *g_buffer = nullptr;
static volatile size_t g_buf_write = 0;
static volatile size_t g_buf_count = 0;
static volatile bool g_ready_for_inference = false;

// allocate on init
void initVoice() {
  // allocate circular buffer of RAW_SAMPLE_COUNT
  size_t capacity = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  g_buffer = (int16_t*)malloc(sizeof(int16_t) * capacity);
  if (!g_buffer) {
    Serial.println("[Voice] failed to allocate audio buffer");
    return;
  }
  memset((void*)g_buffer, 0, sizeof(int16_t) * capacity);
  g_buf_write = 0;
  g_buf_count = 0;
  g_ready_for_inference = false;

  randomSeed(millis());
  Serial.println("[Voice] Initialized (streaming -> EI)");
}

/**
 * Called from web_control when it receives binary frames (PCM16 LE)
 */
void microphone_feed(const int16_t *samples, size_t count) {
  if (!g_buffer) return;
  const size_t capacity = EI_CLASSIFIER_RAW_SAMPLE_COUNT;

  // append samples into circular buffer; maintain count as min(capacity, running_count)
  for (size_t i = 0; i < count; i++) {
    g_buffer[g_buf_write] = samples[i];
    g_buf_write++;
    if (g_buf_write >= capacity) g_buf_write = 0;
    if (g_buf_count < capacity) g_buf_count++;
  }

  // if we have a full window, mark ready (we will run classifier in voiceLoop)
  if (g_buf_count >= capacity) {
    g_ready_for_inference = true;
  }
}

/**
 * signal.get_data callback for EI API.
 * Converts the circular int16 buffer into float samples for the classifier.
 * offset: offset in the signal window to read
 * length: how many samples to write into out_ptr
 */
static int ei_signal_get_data(size_t offset, size_t length, float *out_ptr) {
  const size_t capacity = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  // Determine start index: the classifier expects data in [0 .. window-1].
  // Map that to our circular buffer where g_buf_write points to next write.
  // The last capacity samples are the window. The start index for sample 0 is:
  size_t start;
  if (g_buf_write >= capacity) start = g_buf_write - capacity;
  else start = (capacity + g_buf_write) - capacity; // effectively 0

  // But since g_buf_write always in [0,capacity), the window start is (g_buf_write) position as the next write.
  // The last 'capacity' samples occupy indices (g_buf_write .. g_buf_write+capacity-1) modulo capacity.
  // For offset x, index = (g_buf_write + offset) % capacity
  for (size_t i = 0; i < length; i++) {
    size_t idx = (g_buf_write + offset + i) % capacity;
    int16_t v = g_buffer[idx];
    out_ptr[i] = (float)v / 32768.0f;
  }
  return 0;
}

/**
 * Convert model results -> call handleVoiceCommand (re-uses your mapping)
 */
static void process_classification(ei_impulse_result_t *result) {
  // find the highest scoring label
  float best_score = 0.0f;
  size_t best_ix = SIZE_MAX;
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    float val = result->classification[i].value;
    if (val > best_score) { best_score = val; best_ix = i; }
  }

  if (best_ix == SIZE_MAX) return;

  // label string:
  String label = String(result->classification[best_ix].label);
  // Emit to web clients and run mapping
  String out = "VOICE_RX:" + label;
  notifyClients(out);

  // For debugging
  Serial.printf("[Voice] EI label='%s' (score=%.3f)\n", result->classification[best_ix].label, best_score);

  // If score is reasonably confident, perform the command mapping
  const float CONF_THRESHOLD = 0.50f; // tune this: 0.5..0.8
  if (best_score >= CONF_THRESHOLD) {
    handleVoiceCommand(label); // uses your existing mapping that sets ws_setDirection/etc.
  }
}

/**
 * Main voice loop — called from main loop() frequently.
 * If we have at least EI_CLASSIFIER_RAW_SAMPLE_COUNT samples, run classifier.
 */
void voiceLoop() {
  if (!g_ready_for_inference) return;
  g_ready_for_inference = false;

  // Prepare signal_t
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  signal.get_data = &ei_signal_get_data;

  ei_impulse_result_t result = {0};

  // run classifier (non-continuous API)
  EI_IMPULSE_ERROR r = run_classifier(&signal, &result, false);
  if (r != EI_IMPULSE_OK) {
    Serial.printf("[Voice] run_classifier error: %d\n", r);
    return;
  }

  process_classification(&result);
  // Optionally produce a beep feedback when something happened
  playClickBeep();
}
