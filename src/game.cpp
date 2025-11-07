#include "game.h"
#include "display.h"
#include "buzzer.h"
#include <Arduino.h>
#include <Adafruit_ILI9341.h>


uint8_t xs[MAX_LEN], ys[MAX_LEN];
uint16_t snake_len = 3;
int8_t direction = 1;
bool food_eaten = true;
uint8_t food_x = 0, food_y = 0;
bool game_over = false;
bool paused = false;
int score = 0;
int level = 1;
int snake_speed = 220;

void safeSpawnFood() {
  if (!food_eaten) return;

  int gridX = PLAY_W / CELL;
  int gridY = PLAY_H / CELL;
  bool placed = false;
  int attempts = 0;

  while (!placed) {
    attempts++;
    if (attempts > 2000) {
      // fallback: scan for the first free cell
      for (uint8_t yy = 0; yy < gridY && !placed; yy++) {
        for (uint8_t xx = 0; xx < gridX && !placed; xx++) {
          bool onSnake = false;
          for (int i = 0; i < snake_len; i++) if (xs[i] == xx && ys[i] == yy) { onSnake = true; break; }
          if (!onSnake) {
            food_x = xx;
            food_y = yy;
            food_eaten = false;
            placed = true;
            Serial.printf("Food (fallback) at %d,%d\n", food_x, food_y);
          }
        }
      }
      break;
    }

    uint8_t fx = random(0, gridX);
    uint8_t fy = random(0, gridY);
    bool onSnake = false;
    for (int i = 0; i < snake_len; i++) if (xs[i] == fx && ys[i] == fy) { onSnake = true; break; }
    if (!onSnake) {
      food_x = fx;
      food_y = fy;
      food_eaten = false;
      placed = true;
      Serial.printf("Food at %d,%d\n", food_x, food_y);
    }
  }

  if (placed) {
    // draw fruit immediately so it appears on screen
    draw_fruit_cell(food_x, food_y);
  }
}



// Incremental movement: erase tail, move head, draw head/body, handle eating & collisions.
void move_snake() {
  // compute next head position
  int next_x = xs[0];
  int next_y = ys[0]; 
  switch (direction) {
    case 0: next_y = ys[0] - 1; break; // Up
    case 1: next_x = xs[0] + 1; break; // Right
    case 2: next_y = ys[0] + 1; break; // Down
    case 3: next_x = xs[0] - 1; break; // Left
  }

  int maxX = PLAY_W / CELL;
  int maxY = PLAY_H / CELL;

  // Check wall collision
  if (next_x < 0 || next_x >= maxX || next_y < 0 || next_y >= maxY) {
    Serial.println("Collision: wall");
    playGameOverBeep();
    game_over = true;
    draw_game_over_screen();
    return;
  }

  // Determine if we will grow (if we eat fruit)
  bool willGrow = (next_x == (int)food_x && next_y == (int)food_y);

  // Self collision check:
  // moving into the current tail cell is allowed if we are NOT growing,
  // because the tail will move away this tick.
  for (int i = 0; i < snake_len; ++i) {
    // If checking tail cell and we will not grow, skip the tail index
    if (!willGrow && i == snake_len - 1) continue;
    if (xs[i] == next_x && ys[i] == next_y) {
      Serial.println("Collision: self");
      playGameOverBeep();
      game_over = true;
      draw_game_over_screen();
      return;
    }
  }

  // Save old tail coords (before shifting) so we can erase if needed / re-add if growing
  int old_tail_x = xs[snake_len - 1];
  int old_tail_y = ys[snake_len - 1];

  // Shift body (move each segment up)
  for (int i = snake_len - 1; i > 0; --i) {
    xs[i] = xs[i - 1];
    ys[i] = ys[i - 1];
  }

  // Place new head
  xs[0] = next_x;
  ys[0] = next_y;

  // If we will grow: re-append the old tail (grow by 1)
  if (willGrow) {
    if (snake_len < MAX_LEN) {
      // place new extra tail segment at saved old tail coordinates
      xs[snake_len] = old_tail_x;
      ys[snake_len] = old_tail_y;
      ++snake_len;
    }
    // Update score/level/speed
    food_eaten = true;
    score++;
    level = score / 3 + 1;
    int base = 440;//220
    int decrement = (level - 1) * 12;
    snake_speed = max(70, base - decrement);
    Serial.printf("Ate fruit: score=%d len=%d speed=%d level=%d\n", score, snake_len, snake_speed, level);
    playEatBeep();

    // Erase fruit cell so there's no leftover
    int fx = PLAY_X + food_x * CELL;
    int fy = PLAY_Y + food_y * CELL;
    tft.fillRect(fx, fy, CELL, CELL, ILI9341_BLACK);

    // Draw new head
    int hx = PLAY_X + xs[0] * CELL;
    int hy = PLAY_Y + ys[0] * CELL;
    tft.fillRect(hx, hy, CELL, CELL, ILI9341_GREEN);

    // Draw the immediate body segment (rounded)
    if (snake_len > 1) {
      int bx = PLAY_X + xs[1] * CELL;
      int by = PLAY_Y + ys[1] * CELL;
      tft.fillRoundRect(bx, by, CELL, CELL, 2, ILI9341_YELLOW);
    }

    // We return; safeSpawnFood() will be called by the loop after this if food_eaten==true
    return;
  }

  // Normal move (not growing):
  // Erase old tail cell (background)
  int tx = PLAY_X + old_tail_x * CELL;
  int ty = PLAY_Y + old_tail_y * CELL;
  tft.fillRect(tx, ty, CELL, CELL, ILI9341_BLACK);

  // Draw new head
  int hx = PLAY_X + xs[0] * CELL;
  int hy = PLAY_Y + ys[0] * CELL;
  tft.fillRect(hx, hy, CELL, CELL, ILI9341_GREEN);

  // Draw the immediate body segment (rounded)
  if (snake_len > 1) {
    int bx = PLAY_X + xs[1] * CELL;
    int by = PLAY_Y + ys[1] * CELL;
    tft.fillRoundRect(bx, by, CELL, CELL, 2, ILI9341_YELLOW);
  }
}



void restart_with_splash()
{
  // display shows splash & countdown
  showSplashAndCountdown();

  // reinitialize game
  snake_len = 3;
  score = 0;
  level = 1;
  snake_speed = 440;//220
  direction = random(0, 4);
  uint8_t hx = (PLAY_W / CELL) / 2;
  uint8_t hy = (PLAY_H / CELL) / 2;
  for (int i = 0; i < snake_len; i++)
  {
    xs[i] = hx - i;
    ys[i] = hy;
  }
  game_over = false;
  paused = false;
  food_eaten = true;
  safeSpawnFood();
  draw_frame();
}