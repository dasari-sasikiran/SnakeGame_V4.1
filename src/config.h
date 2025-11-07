#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Screen dimensions (use types that fit 320)
constexpr uint16_t SCREEN_W = 240;
constexpr uint16_t SCREEN_H = 320;


// Layout
constexpr uint8_t  HUD_H   = 20;
constexpr uint16_t PLAY_X  = 0;
constexpr uint16_t PLAY_Y  = HUD_H;
constexpr uint16_t PLAY_W  = SCREEN_W;
constexpr uint16_t PLAY_H  = SCREEN_H - HUD_H;

constexpr uint8_t CELL    = 10;
constexpr uint8_t CELL_SP = 2;

// Pins (ESP32 mapping for v4.1)
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  23
#define TFT_MISO  19
#define TFT_SCK   18

#define BUZZER_PIN 27
#define IR_PIN     15

// WiFi placeholders (declare extern in one .cpp if used)
extern const char *ssid;
extern const char *password;

// IR codes (keep the same values you use)
#define IR_UP_CODE           0xC00058
#define IR_DOWN_CODE         0xC00059
#define IR_LEFT_CODE         0xC0005A
#define IR_RIGHT_CODE        0xC0005B
#define IR_PAUSE_CODE        0xC0005C
#define IR_RESET_CODE        0xC0000C
#define IR_SOUND_TOGGLE_CODE 0xC0000D

// Game constants
#define MAX_LEN 300

#endif // CONFIG_H
