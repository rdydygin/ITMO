/* Snake application for the SDK_FreeRTOS project (CMSIS-RTOS v1). */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "main.h"
#include "snake.h"
#include "keyboard.h"
#include "oled.h"
#include "buzzer.h"
#include <stdio.h>

enum { CMD_UP, CMD_RIGHT, CMD_DOWN, CMD_LEFT, CMD_PAUSE, CMD_RESTART };
enum { SOUND_START, SOUND_PAUSE, SOUND_RESTART, SOUND_FOOD, SOUND_LOST, SOUND_WON };
static QueueHandle_t commands, frames, sounds;
static SemaphoreHandle_t i2c_mutex;
static SnakeGame game; /* Only GameTask owns the mutable game state. */
static void InputTask(void const *argument);
static void GameTask(void const *argument);
static void DisplayTask(void const *argument);
static void SoundTask(void const *argument);
static void sound_event(uint8_t event) { (void)xQueueSend(sounds, &event, 0); }
static StaticTask_t idle_tcb;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack, uint32_t *size) {
    *tcb = &idle_tcb; *stack = idle_stack; *size = configMINIMAL_STACK_SIZE;
}
void MX_FREERTOS_Init(void) {
    commands = xQueueCreate(8, sizeof(uint8_t));
    frames = xQueueCreate(1, sizeof(SnakeGame));
    sounds = xQueueCreate(8, sizeof(uint8_t));
    i2c_mutex = xSemaphoreCreateMutex();
    configASSERT(commands && frames && sounds && i2c_mutex);
    osThreadDef(Input, InputTask, osPriorityAboveNormal, 0, 256);
    osThreadDef(Game, GameTask, osPriorityNormal, 0, 256);
    osThreadDef(Display, DisplayTask, osPriorityBelowNormal, 0, 512);
    osThreadDef(Sound, SoundTask, osPriorityBelowNormal, 0, 256);
    configASSERT(osThreadCreate(osThread(Sound), NULL));
    configASSERT(osThreadCreate(osThread(Input), NULL));
    configASSERT(osThreadCreate(osThread(Game), NULL));
    configASSERT(osThreadCreate(osThread(Display), NULL));
}
static void InputTask(void const *argument) {
    (void)argument;
    uint16_t candidate = 0, stable = 0;
    uint8_t samples = 0;
    TickType_t wake = xTaskGetTickCount();
    const uint8_t bits[] = {1, 5, 7, 3, 4, 9}; /* 2,6,8,4,5,* */
    for (;;) {
        uint16_t keys = 0;
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        HAL_StatusTypeDef status = keyboard_read(&keys);
        xSemaphoreGive(i2c_mutex);
        if (status == HAL_OK) {
            if (keys != candidate) { candidate = keys; samples = 1; }
            else if (samples < 3) ++samples;
            if (samples == 3 && candidate != stable) {
                uint16_t pressed = candidate & (uint16_t)~stable;
                stable = candidate;
                /* Treat multiple held keys as ambiguous; no phantom turn. */
                if (stable && !(stable & (stable - 1u))) {
                    for (uint8_t cmd = 0; cmd < sizeof(bits); ++cmd)
                        if (pressed & (1u << bits[cmd]))
                            (void)xQueueSend(commands, &cmd, 0);
                }
            }
        } else { samples = 0; } /* A failed read is not a key release. */
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}
static void GameTask(void const *argument) {
    (void)argument;
    snake_init(&game, xTaskGetTickCount() ^ 0x12345678u);
    game.status = SNAKE_PAUSED; /* Let the player get ready before movement. */
    xQueueOverwrite(frames, &game);
    TickType_t wake = xTaskGetTickCount(), last_step = wake;
    for (;;) {
        uint8_t command;
        int changed = 0;
        TickType_t now = xTaskGetTickCount();
        while (xQueueReceive(commands, &command, 0) == pdPASS) {
            if (command <= CMD_LEFT) snake_turn(&game, (SnakeDirection)command);
            else if (command == CMD_PAUSE) {
                SnakeStatus before = game.status;
                snake_pause(&game); last_step = now; changed = 1;
                if (game.status != before)
                    sound_event(game.status == SNAKE_PAUSED ? SOUND_PAUSE : SOUND_START);
            }
            else if (command == CMD_RESTART) {
                snake_init(&game, game.rng ^ now); game.status = SNAKE_PAUSED;
                last_step = now; changed = 1;
                xQueueReset(sounds); sound_event(SOUND_RESTART);
                xQueueReset(commands); break;
            }
        }
        if (game.status == SNAKE_RUNNING && now - last_step >= pdMS_TO_TICKS(snake_period_ms(&game))) {
            uint16_t score = game.score;
            snake_step(&game); last_step = now; changed = 1;
            if (game.status == SNAKE_LOST) sound_event(SOUND_LOST);
            else if (game.status == SNAKE_WON) sound_event(SOUND_WON);
            else if (game.score != score) sound_event(SOUND_FOOD);
        }
        if (changed) xQueueOverwrite(frames, &game);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}
static void draw_cell(SnakeCell c, int filled) {
    uint8_t x = (uint8_t)(4 + c.x * 4), y = (uint8_t)(14 + c.y * 4);
    for (uint8_t dy = 0; dy < 3; ++dy)
        for (uint8_t dx = 0; dx < 3; ++dx)
            if (filled || dx == 1 || dy == 1) oled_DrawPixel(x + dx, y + dy, White);
}
static void draw_frame(const SnakeGame *g) {
    char text[19];
    oled_Fill(Black);
    const char *status = g->status == SNAKE_PAUSED ? "PAUSE" :
                         g->status == SNAKE_LOST ? "LOST *" :
                         g->status == SNAKE_WON ? "WIN *" : "SNAKE";
    snprintf(text, sizeof(text), "%s %u", status, (unsigned)g->score);
    oled_SetCursor(0, 0); oled_WriteString(text, Font_7x10, White);
    oled_DrawSquare(3, 124, 13, 62, White);
    if (g->status != SNAKE_WON) draw_cell(g->food, 0);
    for (uint16_t i = 0; i < g->length; ++i) draw_cell(g->body[i], 1);
    /* Distinguish the head by its dark center. */
    oled_DrawPixel(5 + g->body[0].x * 4, 15 + g->body[0].y * 4, Black);
}
static void DisplayTask(void const *argument) {
    (void)argument;
    SnakeGame frame;
    for (;;) {
        xQueueReceive(frames, &frame, portMAX_DELAY);
        draw_frame(&frame); /* OLED buffer is owned only by this task. */
        for (uint8_t page = 0; page < 8; ++page) {
            xSemaphoreTake(i2c_mutex, portMAX_DELAY);
            oled_UpdatePage(page);
            xSemaphoreGive(i2c_mutex);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

static void SoundTask(void const *argument) {
    (void)argument;
    static const uint16_t tones[][4] = {
        {523, 784, 0, 0}, {392, 0, 0, 0}, {659, 523, 0, 0},
        {1047, 1568, 0, 0}, {440, 330, 220, 0}, {523, 659, 784, 1047}
    };
    for (;;) {
        uint8_t event;
        xQueueReceive(sounds, &event, portMAX_DELAY);
        if (event > SOUND_WON) continue;
        for (unsigned i = 0; i < 4 && tones[event][i]; ++i) {
            Buzzer_Set_Freq(tones[event][i]);
            vTaskDelay(pdMS_TO_TICKS(event == SOUND_LOST ? 120 : 70));
            Buzzer_Set_Freq(0);
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }
}
