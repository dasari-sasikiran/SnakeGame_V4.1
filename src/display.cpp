#include "display.h"
#include "game.h"
#include "buzzer.h"
#include "config.h"

#include <Adafruit_ILI9341.h>
#include <TJpg_Decoder.h>
#include "SPIFFS.h"

// TFT instance (use pins from config.h)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// wrapper to let old display.* API calls still work: (if your code used `display` object,
// replace calls accordingly or update the rest of the code to call tft.* functions)
void initDisplay()
{
  // init SPIFFS first (display splash reads file)
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
  }
  else
  {
    Serial.println("SPIFFS mounted");
  }

  tft.begin();
  tft.setRotation(0); // Portrait (default, 240x320)
  tft.fillScreen(ILI9341_BLACK);
}

// ---------- helper used by TJpg_Decoder ----------
/*
 TJpg_Decoder calls your render function to draw pixel blocks.
 For Adafruit_ILI9341 we implement the callback in the standard way:
*/
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  // Safety: ensure rectangle is within bounds (TJpgDec normally handles)
  if (x < 0 || y < 0 || x + w > (int)SCREEN_W || y + h > (int)SCREEN_H)
  {
    // clip / ignore if totally outside
  }

  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  tft.writePixels(bitmap, w * h);
  tft.endWrite();

  return true; // continue decoding
}

// Draw JPG from SPIFFS: filename must start with '/'
void drawJpegFromSPIFFS(const char *filename, int16_t x = 0, int16_t y = 0)
{
  if (!SPIFFS.exists(filename))
  {
    Serial.printf("JPG not found: %s\n", filename);
    return;
  }
  TJpgDec.setJpgScale(1);          // no scaling (1 = full size), adjust if necessary
  TJpgDec.setCallback(tft_output); // set output callback
  Serial.printf("Rendering JPG %s\n", filename);
  TJpgDec.drawFsJpg(x, y, filename); // draw from SPIFFS (works with Bodmer TJpg_Decoder)
}

// --- Keep the old draw helpers but adapted to color TFT ---
// You can rename / reuse your old functions; here I provide versions that call tft.

int textWidthChars(const char *s, uint8_t textSize)
{
  int len = strlen(s);
  return len * 6 * textSize; // approximate same as old code
}

void drawBoldRect(int x, int y, int w, int h)
{
  tft.drawRect(x, y, w, h, ILI9341_WHITE);
  tft.drawRect(x + 1, y + 1, w - 2, h - 2, ILI9341_WHITE);
}
void drawThinHUDBorder() { tft.drawRect(0, 0, SCREEN_W, HUD_H, ILI9341_WHITE); }

void drawPauseIcon(int x, int y) { 
  tft.fillRect(x, y, 6, 12, ILI9341_WHITE);
  tft.fillRect(x+10, y, 6, 12, ILI9341_WHITE);
   /* you can fine tune */ }
void drawPlayIcon(int x, int y) { tft.fillTriangle(x, y, x, y + 12, x + 10, y + 6, ILI9341_WHITE); }

void drawSpeakerOnIcon(int x, int y)
{
  // Speaker body (a small trapezoid/rectangle)
  tft.fillRect(x, y + 3, 4, 10, ILI9341_WHITE);               // rectangle part
  tft.fillTriangle(x + 4, y + 3, x + 4, y + 12, x + 9, y + 7, ILI9341_WHITE); // cone

  // Sound waves (two arcs/curves)
  tft.drawLine(x + 11, y + 5, x + 13, y + 7, ILI9341_WHITE);
  tft.drawLine(x + 13, y + 7, x + 11, y + 10, ILI9341_WHITE);

  tft.drawLine(x + 15, y + 3, x + 18, y + 7, ILI9341_WHITE);
  tft.drawLine(x + 18, y + 7, x + 15, y + 12, ILI9341_WHITE);
}

void drawSpeakerMutedIcon(int x, int y)
{
  // Base speaker (same as ON)
  drawSpeakerOnIcon(x, y);

  // Mute cross (X over the speaker)
  tft.drawLine(x, y, x + 18, y + 16, ILI9341_RED);
  tft.drawLine(x + 18, y, x, y + 16, ILI9341_RED);
}


void draw_cell_fill(uint8_t gx, uint8_t gy)
{
  int px = PLAY_X + gx * CELL;
  int py = PLAY_Y + gy * CELL;
  if (px < 0 || py < 0 || px + CELL > SCREEN_W || py + CELL > SCREEN_H)
    return;
  tft.fillRect(px, py, CELL, CELL, ILI9341_WHITE);
}

void draw_fruit_cell(uint8_t gx, uint8_t gy)
{
  int px = PLAY_X + gx * CELL;
  int py = PLAY_Y + gy * CELL;
  if (px < 0 || py < 0 || px + CELL > SCREEN_W || py + CELL > SCREEN_H)
    return;

  // Ensure the previous contents are cleared
  tft.fillRect(px, py, CELL, CELL, ILI9341_BLACK);

  int cx = px + CELL / 2;
  int cy = py + CELL / 2;
  int r = max(1, (int)CELL / 2);
  tft.fillCircle(cx, cy, r-1, ILI9341_WHITE);
}



void draw_HUD()
{
  // Draw HUD background
  tft.fillRect(0, 0, SCREEN_W, HUD_H, ILI9341_BLUE);

  // Draw text
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.setTextSize(2);
  tft.setCursor(4, 2);
  tft.printf("LVL:%d SCORE:%d", level, score);

  // Clear and draw pause/play icon area (prevent leftover)
  int iconX = SCREEN_W - 60;
  int iconY = 2;
  tft.fillRect(iconX, iconY, 20, HUD_H - 4, ILI9341_BLUE); // clear icon area
  if (paused)
    drawPlayIcon(iconX, iconY);
  else
    drawPauseIcon(iconX, iconY);

  // Clear and draw sound icon area
  tft.fillRect(SCREEN_W - 28, 2, 26, HUD_H - 4, ILI9341_BLUE);
  if (sound_enabled)
    drawSpeakerOnIcon(SCREEN_W - 20, 2);
  else
    drawSpeakerMutedIcon(SCREEN_W - 20, 2);

  drawThinHUDBorder();
}



void draw_playfield()
{
  draw_fruit_cell(food_x, food_y);
  for (int i = 0; i < snake_len; i++)
  {
    int gx = xs[i], gy = ys[i];
    int px = PLAY_X + gx * CELL, py = PLAY_Y + gy * CELL;
    if (i == 0)
      tft.fillRect(px, py, CELL, CELL, ILI9341_GREEN);
    else
      tft.fillRoundRect(px, py, CELL, CELL, 2, ILI9341_YELLOW);
  }
}

void draw_playfield_border() { drawBoldRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H); }

void draw_frame()
{
  // Update HUD and border (lightweight)
  draw_HUD();
  draw_playfield_border();

  // If paused -> draw pause overlay; else clear overlay area (so previous pause bars are erased)
  int cx = PLAY_X + PLAY_W / 2;
  int cy = PLAY_Y + PLAY_H / 2;
  int bar_w = 12, bar_h = 36, gap = 16;
  int overlay_x = cx - gap/2 - bar_w;
  int overlay_w = bar_w * 2 + gap;
  int overlay_y = cy - bar_h/2;
  int overlay_h = bar_h;

  // if (paused) {
  //   tft.fillRect(overlay_x, overlay_y, bar_w, bar_h, ILI9341_WHITE);
  //   tft.fillRect(overlay_x + bar_w + gap, overlay_y, bar_w, bar_h, ILI9341_WHITE);
  // } else {
  //   // clear the overlay region (only this rectangle) with playfield background color
  //   tft.fillRect(overlay_x, overlay_y, overlay_w, overlay_h, ILI9341_BLACK);
  // }

  // Note: snake & fruit are managed incrementally by move_snake() and safeSpawnFood()
}



// game over screen
void draw_game_over_screen()
{
  tft.fillScreen(ILI9341_BLACK);   // black background looks cleaner for game over

  // Title: GAME OVER
  const char *title = "GAME OVER";
  tft.setTextSize(4);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  int titleWidth = strlen(title) * 6 * 4;
  int titleX = (SCREEN_W - titleWidth) / 2;
  int titleY = SCREEN_H / 4;
  tft.setCursor(titleX, titleY);
  tft.print(title);

  // Final Score
  char buf[32];
  snprintf(buf, sizeof(buf), "FINAL SCORE: %d", score);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  int scoreWidth = strlen(buf) * 6 * 2;
  int scoreX = (SCREEN_W - scoreWidth) / 2;
  int scoreY = titleY + 50;
  tft.setCursor(scoreX, scoreY);
  tft.print(buf);

  // Restart prompt
  const char *restart = "PRESS RESTART";
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  int restartWidth = strlen(restart) * 6 * 2;
  int restartX = (SCREEN_W - restartWidth) / 2;
  int restartY = SCREEN_H - 60;
  tft.setCursor(restartX, restartY);
  tft.print(restart);
}


// splash with JPEG support
void showSplashAndCountdown()
{
  // prefer JPEG (logo.jpg in SPIFFS data/)
  tft.fillScreen(ILI9341_BLACK);
  draw_HUD(); // draw HUD background so HUD area is not black while jpg renders
  const char *jpg = "/logo.jpg";
  if (SPIFFS.exists(jpg))
  {
    drawJpegFromSPIFFS(jpg, 0, 0); // splash image
    delay(1400);
  }
  else
  {
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(40, 90);
    tft.print("HUNGRY SNAKE");
    delay(1400);
  }

  playStartupBeep();

  // countdown (3..1)
  for (int n = 3; n >= 1; --n)
  {
    // clear only the playfield, not entire width
    tft.fillRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, ILI9341_BLACK);
    draw_HUD();
    draw_playfield_border();

    tft.setTextSize(6);
    tft.setTextColor(ILI9341_WHITE);
    int cx = PLAY_X + (PLAY_W / 2) - 10;
    int cy = PLAY_Y + (PLAY_H / 2) - 20;
    tft.setCursor(cx, cy);
    tft.printf("%d", n);

    playClickBeep();
    delay(1000);
  }

  // Go!
  tft.fillRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, ILI9341_BLACK);
  draw_HUD();
  draw_playfield_border();

  tft.setTextSize(4);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(PLAY_X + PLAY_W / 2 - 32, PLAY_Y + PLAY_H / 2 - 14);
  tft.print("Go!");
  playClickBeep();
  delay(800);

  // final clear before starting game
  tft.fillRect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, ILI9341_BLACK);
  draw_frame();
}
