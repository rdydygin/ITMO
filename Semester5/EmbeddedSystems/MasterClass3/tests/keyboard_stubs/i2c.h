#include "keyboard.h"
#define I2C_MEMADD_SIZE_8BIT 1
extern int hi2c1;
HAL_StatusTypeDef HAL_I2C_Mem_Write(int *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(int *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t, uint32_t);
