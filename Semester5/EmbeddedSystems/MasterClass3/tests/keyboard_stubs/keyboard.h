#ifndef MOCK_KEYBOARD_H
#define MOCK_KEYBOARD_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
HAL_StatusTypeDef keyboard_read(uint16_t *keys);
#endif
