#ifndef MOCK_HAL_H
#define MOCK_HAL_H
#include <stdint.h>
typedef struct { int unused; } I2C_HandleTypeDef;
void HAL_Delay(uint32_t ms);
int HAL_I2C_Mem_Write(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t, uint32_t);
#endif
