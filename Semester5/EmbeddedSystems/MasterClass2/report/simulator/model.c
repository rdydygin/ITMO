#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calculator.h"
#include "oled.h"
I2C_HandleTypeDef hi2c1;
static unsigned page;
static uint8_t framebuffer[1024];
void HAL_Delay(uint32_t ms) { (void)ms; }
int HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t addr, uint16_t reg, uint16_t size, uint8_t *data, uint16_t len, uint32_t timeout) {
    (void)h; (void)addr; (void)size; (void)timeout;
    if (reg == 0 && len == 1 && data[0] >= 0xB0 && data[0] <= 0xB7) page = data[0] - 0xB0;
    if (reg == 0x40 && len == 128) memcpy(framebuffer + page * 128, data, 128);
    return 0;
}
static Calculator calculator;

static void DisplayLine(uint8_t y, char *text) {
    oled_SetCursor(0, y);
    oled_WriteString(text, Font_7x10, White);
}

static void Calculator_Display(void) {
    char text[19];
    oled_Fill(Black);
    DisplayLine(0, "INTEGER CALCULATOR");
    snprintf(text, sizeof(text), "A: %s%ld", calculator.a == 0 && !calculator.operand && calculator.negative ? "-" : "", (long)calculator.a);
    DisplayLine(12, text);
    snprintf(text, sizeof(text), "%c B: %s%ld", calculator.op, calculator.b == 0 && calculator.operand && calculator.negative ? "-" : "", (long)calculator.b);
    DisplayLine(24, text);
    if (calculator.error) DisplayLine(36, "OVERFLOW: hold *");
    else if (calculator.done) {
        snprintf(text, sizeof(text), "= %ld", (long)calculator.result);
        DisplayLine(36, text);
    } else DisplayLine(36, calculator.operand ? "Enter B, # =" : "Enter A, * op");
    DisplayLine(48, "* op  # =");
    
    oled_UpdateScreen();
}


int main(int argc, char **argv) {
    if (argc != 3) return 1;
    oled_Init(); Calculator_Clear(&calculator);
    for (const char *p = argv[1]; *p; ++p) {
        if (*p == 'C') Calculator_Clear(&calculator);
        else if (*p == 'N') Calculator_Negate(&calculator);
        else Calculator_Key(&calculator, *p);
    }
    Calculator_Display();
    FILE *f = fopen(argv[2], "wb"); if (!f) return 2;
    fprintf(f, "P5\n128 64\n255\n");
    for (unsigned y = 0; y < 64; ++y)
        for (unsigned x = 0; x < 128; ++x)
            fputc((framebuffer[x + (y/8)*128] & (1 << (y%8))) ? 255 : 0, f);
    fclose(f);
    printf("a=%ld b=%ld op=%c done=%u error=%u result=%ld\n", (long)calculator.a, (long)calculator.b, calculator.op, calculator.done, calculator.error, (long)calculator.result);
    return 0;
}
