#ifndef SNAKE_H
#define SNAKE_H
#include <stdint.h>
#define SNAKE_COLS 30
#define SNAKE_ROWS 12
#define SNAKE_CAPACITY (SNAKE_COLS * SNAKE_ROWS)
typedef struct { uint8_t x, y; } SnakeCell;
typedef enum { SNAKE_UP, SNAKE_RIGHT, SNAKE_DOWN, SNAKE_LEFT } SnakeDirection;
typedef enum { SNAKE_RUNNING, SNAKE_PAUSED, SNAKE_LOST, SNAKE_WON } SnakeStatus;
typedef struct {
    SnakeCell body[SNAKE_CAPACITY], food;
    uint16_t length, score;
    SnakeDirection direction;
    SnakeStatus status;
    uint32_t rng;
    uint8_t turned;
} SnakeGame;
void snake_init(SnakeGame *game, uint32_t seed);
void snake_turn(SnakeGame *game, SnakeDirection direction);
void snake_pause(SnakeGame *game);
void snake_step(SnakeGame *game);
uint32_t snake_period_ms(const SnakeGame *game);
#endif
