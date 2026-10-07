/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "kb.h"
#include "sdk_uart.h"
#include "pca9538.h"
#include "oled.h"
#include "fonts.h"
#include "calculator.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Calculator_Run(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  oled_Init();

  /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  Calculator_Run();
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static Calculator calculator;

static void DisplayLine(uint8_t y, char *text) {
    oled_SetCursor(0, y);
    oled_WriteString(text, Font_7x10, White);
}

static void Calculator_Display(void) {
    char text[19];
    oled_Fill(Black);
    DisplayLine(0, "INTEGER CALCULATOR");
    snprintf(text, sizeof(text), "A: %s%ld", calculator.a == 0 && !calculator.operand && calculator.negative ? "-" : "", (long)calculator.a);
    DisplayLine(12, text);
    snprintf(text, sizeof(text), "%c B: %s%ld", calculator.op, calculator.b == 0 && calculator.operand && calculator.negative ? "-" : "", (long)calculator.b);
    DisplayLine(24, text);
    if (calculator.error) DisplayLine(36, "OVERFLOW: hold *");
    else if (calculator.done) {
        snprintf(text, sizeof(text), "= %ld", (long)calculator.result);
        DisplayLine(36, text);
    } else DisplayLine(36, calculator.operand ? "Enter B, # =" : "Enter A, * op");
    DisplayLine(48, "* op  # =");
    
    oled_UpdateScreen();
}

/* Physical rows from top to bottom: 123 / 456 / 789 / *0#. */
static char Keyboard_Scan(void) {
    static const uint8_t rows[4] = {ROW1, ROW2, ROW3, ROW4};
    static const char keys[4][3] = {{'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'*','0','#'}};
    char pressed = 0;
    for (unsigned row = 0; row < 4; ++row) {
        uint8_t value = Check_Row(rows[row]);
        if (value == 0xFF) return 0;
        if (!value) continue;
        if (pressed) return 0; /* Ignore chords across rows. */
        pressed = keys[row][value == 4 ? 0 : value == 2 ? 1 : 2];
    }
    return pressed;
}

void Calculator_Run(void) {
    char candidate = 0, stable = 0;
    uint32_t changed = HAL_GetTick(), pressed_at = 0;
    uint8_t held = 0;
    Calculator_Clear(&calculator);
    Calculator_Display();
    while (1) {
        char key = Keyboard_Scan();
        uint32_t now = HAL_GetTick();
        if (key != candidate) { candidate = key; changed = now; }
        if (candidate != stable && (uint32_t)(now - changed) >= 30) {
            if (stable && !held) {
                Calculator_Key(&calculator, stable);
                Calculator_Display();
            }
            stable = candidate; pressed_at = now; held = 0;
        }
        if (stable && !held && (uint32_t)(now - pressed_at) >= 700) {
            if (stable == '*') { Calculator_Clear(&calculator); held = 1; }
            else if (stable == '#') { Calculator_Negate(&calculator); held = 1; }
            if (held) Calculator_Display();
        }
        HAL_Delay(5);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
