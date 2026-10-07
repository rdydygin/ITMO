#include "keyboard.h"
#include "i2c.h"
#define KEYBOARD_ADDR 0xE2u
static HAL_StatusTypeDef write_reg(uint8_t reg, uint8_t value) {
    return HAL_I2C_Mem_Write(&hi2c1, KEYBOARD_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
}
HAL_StatusTypeDef keyboard_read(uint16_t *keys) {
    uint16_t result = 0;
    for (uint8_t row = 0; row < 4; ++row) {
        uint8_t input = 0xff;
        /* SDK_Keyboard calls Set_Keyboard before EACH Check_Row. */
        if (write_reg(2, 0) != HAL_OK || write_reg(1, 0) != HAL_OK) return HAL_ERROR;
        if (write_reg(3, (uint8_t)~(1u << row)) != HAL_OK) return HAL_ERROR;
        if (HAL_I2C_Mem_Read(&hi2c1, KEYBOARD_ADDR | 1u, 0, I2C_MEMADD_SIZE_8BIT,
                            &input, 1, 100) != HAL_OK) return HAL_ERROR;
        for (uint8_t col = 0; col < 3; ++col)
            if (!(input & (0x10u << col))) result |= 1u << (row * 3 + col);
    }
    *keys = result;
    return HAL_OK;
}
