#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "main.h"

void initKeyboard(void);
char readKey(void);
// Возвращает новое нажатие после подавления дребезга или '\0'.
char scanKeyboard(void);

#endif
