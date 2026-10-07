#ifndef BUZZER_H
#define BUZZER_H
#include "main.h"
/* SDK_Buzzer connection: PA5, TIM2 channel 1. */
void Buzzer_Init(void);
void Buzzer_Set_Freq(uint16_t frequency);
#endif
