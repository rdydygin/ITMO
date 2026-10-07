#include "main.h"
#include "pca9538.h"
#include "kb.h"
#include "sdk_uart.h"
#include "usart.h"

#define KBRD_ADDR 0xE2

HAL_StatusTypeDef Set_Keyboard( void ) {
	HAL_StatusTypeDef ret = HAL_OK;
	uint8_t buf;

	buf = 0;
	ret = PCA9538_Write_Register(KBRD_ADDR, POLARITY_INVERSION, &buf);
	if( ret != HAL_OK ) {
		UART_Transmit((uint8_t*)"Error write polarity\n");
		goto exit;
	}

	buf = 0;
	ret = PCA9538_Write_Register(KBRD_ADDR, OUTPUT_PORT, &buf);
	if( ret != HAL_OK ) {
		UART_Transmit((uint8_t*)"Error write output\n");
	}

exit:
	return ret;
}

uint8_t Check_Row(uint8_t Nrow) {
    uint8_t buf = Nrow;
    if (Set_Keyboard() != HAL_OK) return 0;
    if (PCA9538_Write_Register(KBRD_ADDR, CONFIG, &buf) != HAL_OK) return 0;
    if (PCA9538_Read_Inputs(KBRD_ADDR, &buf) != HAL_OK) return 0;
    switch ((uint8_t)(~buf) & 0x70) {
        case 0x10: return 0x04;
        case 0x20: return 0x02;
        case 0x40: return 0x01;
        case 0: return 0;
        default: return 0xFF; /* Multiple keys: do not accept ambiguous input. */
    }
}
