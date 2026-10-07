#include "snake.h"
#include <string.h>
static int equal(SnakeCell a, SnakeCell b) { return a.x == b.x && a.y == b.y; }
static int occupied(const SnakeGame *g, SnakeCell c, uint16_t count) {
    for (uint16_t i = 0; i < count; ++i) if (equal(g->body[i], c)) return 1;
    return 0;
}
static void spawn_food(SnakeGame *g) {
    /* Bounded search also works when only one free cell remains. */
    g->rng ^= g->rng << 13; g->rng ^= g->rng >> 17; g->rng ^= g->rng << 5;
    const uint16_t food_cells = (SNAKE_COLS - 2) * (SNAKE_ROWS - 2);
    uint16_t start = (uint16_t)(g->rng % food_cells);
    for (uint16_t i = 0; i < food_cells; ++i) {
        uint16_t n = (uint16_t)((start + i) % food_cells);
        SnakeCell c = { (uint8_t)(1 + n % (SNAKE_COLS - 2)),
                        (uint8_t)(1 + n / (SNAKE_COLS - 2)) };
        if (!occupied(g, c, g->length)) { g->food = c; return; }
    }
    g->status = SNAKE_WON;
}
void snake_init(SnakeGame *g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0x9e3779b9u;
    g->length = 3; g->direction = SNAKE_RIGHT; g->status = SNAKE_RUNNING;
    for (uint8_t i = 0; i < 3; ++i) {
        g->body[i].x = (uint8_t)(SNAKE_COLS / 2 - i);
        g->body[i].y = SNAKE_ROWS / 2;
    }
    spawn_food(g);
}
void snake_turn(SnakeGame *g, SnakeDirection d) {
    if (g->status != SNAKE_RUNNING || g->turned || d > SNAKE_LEFT ||
        d == g->direction || (d + 2) % 4 == g->direction) return;
    g->direction = d; g->turned = 1;
}
void snake_pause(SnakeGame *g) {
    if (g->status == SNAKE_RUNNING) g->status = SNAKE_PAUSED;
    else if (g->status == SNAKE_PAUSED) g->status = SNAKE_RUNNING;
}
void snake_step(SnakeGame *g) {
    if (g->status != SNAKE_RUNNING) return;
    int x = g->body[0].x, y = g->body[0].y;
    switch (g->direction) {
    case SNAKE_UP: --y; break;
    case SNAKE_RIGHT: ++x; break;
    case SNAKE_DOWN: ++y; break;
    case SNAKE_LEFT: --x; break;
    }
    g->turned = 0;
    if (x < 0 || x >= SNAKE_COLS || y < 0 || y >= SNAKE_ROWS) {
        g->status = SNAKE_LOST; return;
    }
    SnakeCell next = { (uint8_t)x, (uint8_t)y };
    int eating = equal(next, g->food);
    /* The tail vacates its cell on a non-growing move. */
    if (occupied(g, next, (uint16_t)(g->length - !eating))) {
        g->status = SNAKE_LOST; return;
    }
    if (eating && g->length < SNAKE_CAPACITY) { ++g->length; ++g->score; }
    for (uint16_t i = g->length - 1; i > 0; --i) g->body[i] = g->body[i - 1];
    g->body[0] = next;
    if (eating) spawn_food(g);
}
uint32_t snake_period_ms(const SnakeGame *g) {
    uint32_t speedup = (g->score / 3u) * 15u;
    return speedup >= 150u ? 90u : 240u - speedup;
}
