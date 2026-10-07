#ifndef STUB_HAL_H
#define STUB_HAL_H
#include <stdint.h>
typedef struct { int unused; } I2C_HandleTypeDef;
void HAL_Delay(uint32_t ms);
int HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t addr, uint16_t reg, uint16_t size, uint8_t *data, uint16_t len, uint32_t timeout);
#endif
