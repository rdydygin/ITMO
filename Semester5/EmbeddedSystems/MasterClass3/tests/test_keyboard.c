#include "i2c.h"
#include <assert.h>
#include <stdio.h>
int hi2c1;
static unsigned phase, row, held, failing;
HAL_StatusTypeDef HAL_I2C_Mem_Write(int *bus, uint16_t addr, uint16_t reg, uint16_t size, uint8_t *data, uint16_t n, uint32_t timeout) {
 assert(bus==&hi2c1 && addr==0xE2 && size==1 && n==1 && timeout==100);
 if(failing) return HAL_ERROR;
 assert(phase<3);
 if(phase==0) assert(reg==2 && *data==0);
 if(phase==1) assert(reg==1 && *data==0);
 if(phase==2) assert(reg==3 && *data==(uint8_t)~(1u<<row));
 phase++; return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(int *bus, uint16_t addr, uint16_t reg, uint16_t size, uint8_t *data, uint16_t n, uint32_t timeout) {
 assert(bus==&hi2c1 && addr==0xE3 && reg==0 && size==1 && n==1 && timeout==100);
 assert(phase==3);
 *data=0x70;
 for(unsigned col=0;col<3;col++) if(held&(1u<<(row*3+col))) *data &= ~(0x10u<<col);
 phase=0; row++; return HAL_OK;
}
int main(void) {
 for(unsigned key=0;key<12;key++) {
  row=phase=0; held=1u<<key; uint16_t actual=0;
  assert(keyboard_read(&actual)==HAL_OK && actual==held && row==4);
 }
 row=phase=held=0; uint16_t actual=0xffff;
 assert(keyboard_read(&actual)==HAL_OK && actual==0);
 failing=1; actual=0x123;
 assert(keyboard_read(&actual)==HAL_ERROR && actual==0x123);
 puts("All 12 keys and I2C error handling passed");
}
