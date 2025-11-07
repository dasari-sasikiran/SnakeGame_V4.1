#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include "config.h"

// Single shared TFT instance (defined exactly once in display.cpp)
extern Adafruit_ILI9341 tft;

// Display API used across project (declare the functions you call elsewhere)
void initDisplay();
void draw_frame();
void draw_playfield();
void draw_HUD();
void draw_playfield_border();
void draw_game_over_screen();
void showSplashAndCountdown();
void drawJpegFromSPIFFS(const char *filename, int16_t x, int16_t y);

void draw_fruit_cell(uint8_t gx, uint8_t gy);


#endif // DISPLAY_H
