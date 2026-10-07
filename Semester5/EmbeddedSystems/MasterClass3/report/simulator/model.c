#include "snake.h"
#include "oled.h"
#include <stdio.h>
#include <assert.h>
I2C_HandleTypeDef hi2c1;
static uint8_t buffer[1024], page;
void HAL_Delay(uint32_t ms) {(void)ms;}
int HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t a, uint16_t reg, uint16_t size, uint8_t *data, uint16_t n, uint32_t t) {
 (void)h;(void)a;(void)size;(void)t;
 if(reg==0 && n==1 && (*data & 0xf8)==0xb0) page=*data & 7;
 if(reg==0x40 && n==128) for(unsigned i=0;i<128;i++) buffer[page*128+i]=data[i];
 return 0;
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

static void save(SnakeGame *g, const char *name) {
 draw_frame(g); oled_UpdateScreen();
 char path[128]; snprintf(path,sizeof(path),"report/assets/%s.pgm",name);
 FILE *f=fopen(path,"wb"); assert(f); fprintf(f,"P5\n128 64\n255\n");
 for(unsigned y=0;y<64;y++) for(unsigned x=0;x<128;x++) fputc((buffer[x+128*(y/8)] & (1u<<(y%8)))?255:0,f);
 fclose(f);
}
int main(void) {
 SnakeGame g; snake_init(&g,42); g.status=SNAKE_PAUSED; save(&g,"start");
 snake_pause(&g);
 /* Controlled food location demonstrates growth using snake_step. */
 g.food=(SnakeCell){g.body[0].x+1,g.body[0].y}; snake_step(&g);
 assert(g.score==1 && g.length==4); save(&g,"growth");
 snake_pause(&g); save(&g,"pause");
 snake_pause(&g);
 /* This route reaches the right wall without encountering the new food. */
 while(g.status==SNAKE_RUNNING) snake_step(&g);
 assert(g.status==SNAKE_LOST); save(&g,"loss");
 puts("OLED model: start, growth, pause, wall collision passed");
}
