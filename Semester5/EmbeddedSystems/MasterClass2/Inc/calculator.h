#ifndef CALCULATOR_H
#define CALCULATOR_H
#include <stdint.h>
#include <limits.h>

typedef struct {
    int32_t a, b, result;
    uint8_t operand, entered, negative, done, error;
    char op;
} Calculator;

static inline void Calculator_Clear(Calculator *c) {
    *c = (Calculator){ .op = '+' };
}

static inline void Calculator_Key(Calculator *c, char key) {
    if (key >= '0' && key <= '9') {
        if (c->done || c->error) Calculator_Clear(c);
        int32_t *value = c->operand ? &c->b : &c->a;
        int64_t next = (int64_t)*value * 10 +
                       (c->negative ? -(key - '0') : key - '0');
        if (next < INT32_MIN || next > INT32_MAX) { c->error = 1; return; }
        *value = (int32_t)next;
        c->entered = 1;
    } else if (key == '*') {
        if (c->error) { Calculator_Clear(c); return; }
        if (c->done) {
            c->a = c->result; c->b = 0; c->done = 0;
            c->operand = 1; c->entered = 0; c->negative = 0; c->op = '+';
        } else if (!c->operand) {
            c->operand = 1; c->entered = 0; c->negative = 0; c->op = '+';
        } else if (!c->entered) {
            c->op = c->op == '+' ? '-' : c->op == '-' ? '*' : '+';
        }
    } else if (key == '#') {
        if (c->error || c->done || !c->operand || !c->entered) return;
        int64_t result = c->op == '+' ? (int64_t)c->a + c->b :
                         c->op == '-' ? (int64_t)c->a - c->b :
                                       (int64_t)c->a * c->b;
        if (result < INT32_MIN || result > INT32_MAX) c->error = 1;
        else { c->result = (int32_t)result; c->done = 1; }
    }
}

static inline void Calculator_Negate(Calculator *c) {
    if (c->done || c->error) return;
    int32_t *value = c->operand ? &c->b : &c->a;
    if (*value == INT32_MIN) { c->error = 1; return; }
    *value = -*value;
    c->negative = !c->negative;
}
#endif
