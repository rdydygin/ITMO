#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "main.h"
/* Bits: 0..2 top row, 9..11 bottom row; columns left to right. */
HAL_StatusTypeDef keyboard_read(uint16_t *keys);
#endif
