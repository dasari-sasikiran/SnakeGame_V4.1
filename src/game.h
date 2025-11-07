#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include "config.h"

extern uint8_t xs[MAX_LEN];
extern uint8_t ys[MAX_LEN];
extern uint16_t snake_len;
extern int8_t direction; // 0=Up,1=Right,2=Down,3=Left
extern bool food_eaten;
extern uint8_t food_x;
extern uint8_t food_y;
extern bool game_over;
extern bool paused;
extern int score;
extern int level;
extern int snake_speed;

void safeSpawnFood();
void move_snake();
void restart_with_splash();

#endif // GAME_H