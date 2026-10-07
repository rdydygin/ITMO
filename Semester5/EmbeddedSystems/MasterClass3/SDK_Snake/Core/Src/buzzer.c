/* Adapted from lmtspbru SDK_Buzzer: TIM2_CH1 on PA5.
 * Use a 1 MHz timer clock and 50% PWM; zero frequency mutes output.
 */
#include "buzzer.h"
static TIM_HandleTypeDef timer;
void Buzzer_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef pin = {0};
    pin.Pin = GPIO_PIN_5; pin.Mode = GPIO_MODE_AF_PP;
    pin.Pull = GPIO_NOPULL; pin.Speed = GPIO_SPEED_FREQ_LOW;
    pin.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &pin);
    RCC_ClkInitTypeDef clocks; uint32_t latency;
    HAL_RCC_GetClockConfig(&clocks, &latency);
    uint32_t hz = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) hz *= 2;
    timer.Instance = TIM2;
    timer.Init.Prescaler = hz / 1000000u - 1u;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.Period = 999;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&timer) != HAL_OK) Error_Handler();
    TIM_OC_InitTypeDef channel = {0};
    channel.OCMode = TIM_OCMODE_PWM1;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&timer, &channel, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();
    if (HAL_TIM_PWM_Start(&timer, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
}
void Buzzer_Set_Freq(uint16_t frequency) {
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_1, 0);
    if (!frequency) return;
    uint32_t period = 1000000u / frequency;
    __HAL_TIM_SET_AUTORELOAD(&timer, period - 1u);
    __HAL_TIM_SET_COUNTER(&timer, 0);
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_1, period / 2u);
}
