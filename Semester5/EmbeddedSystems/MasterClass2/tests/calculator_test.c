#include <assert.h>
#include <stdio.h>
#include "calculator.h"
static Calculator c;
static void keys(const char *s) { while (*s) Calculator_Key(&c, *s++); }
int main(void) {
    Calculator_Clear(&c); keys("12*34#"); assert(c.done && c.result == 46);
    Calculator_Clear(&c); keys("12**34#"); assert(c.done && c.result == -22);
    Calculator_Clear(&c); keys("12***34#"); assert(c.done && c.result == 408);
    Calculator_Clear(&c); Calculator_Negate(&c); keys("12*3#"); assert(c.result == -9);
    Calculator_Clear(&c); keys("12***"); Calculator_Negate(&c); keys("3#"); assert(c.result == -36);
    Calculator_Clear(&c); keys("2147483647*1#"); assert(c.error);
    Calculator_Clear(&c); keys("50000***50000#"); assert(c.error);
    Calculator_Clear(&c); keys("2147483648"); assert(c.error);
    Calculator_Clear(&c); Calculator_Negate(&c); keys("2147483648*0#"); assert(c.result == INT32_MIN && !c.error);
    keys("*1#"); assert(c.result == INT32_MIN + 1);
    keys("7"); assert(!c.done && c.a == 7 && !c.operand);
    Calculator_Clear(&c); keys("0***9#"); assert(c.done && c.result == 0);
    Calculator_Clear(&c); keys("1*#"); assert(!c.done);
    puts("Calculator tests passed");
}
