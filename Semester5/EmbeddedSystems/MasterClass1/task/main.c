#include "main.h"
#include "keyboard.h"
#include "tm1637.h"

#define LED_PIN 5U

typedef enum { FIRST_OPERAND, SECOND_OPERAND, RESULT } CalculatorState;

volatile uint32_t tickCount = 0;
static CalculatorState state = FIRST_OPERAND;
static uint8_t hasFirstOperand;
static uint8_t firstOperand;
static uint8_t secondOperand;
static uint8_t hasSecondOperand;
// До первого * операция ещё не выбрана.
static uint8_t operation;
static const char operations[] = {'+', '-', '*', '/'};

void osSystickHandler(void) {
  ++tickCount;
}

static void setLED(uint8_t on) {
  GPIOA->BSRR = on ? (1U << LED_PIN) : (1U << (LED_PIN + 16U));
}

static void initGPIO(void) {
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;
  // Чтение обеспечивает задержку после включения тактирования.
  (void)RCC->IOPENR;
  setLED(0);
  GPIOA->MODER = (GPIOA->MODER & ~(3U << (LED_PIN * 2U))) |
                 (1U << (LED_PIN * 2U));
  GPIOA->OTYPER &= ~(1U << LED_PIN);
}

static void initUSART2(void) {
  RCC->APBENR1 |= RCC_APBENR1_USART2EN;
  (void)RCC->APBENR1;
  GPIOA->MODER = (GPIOA->MODER & ~(0xFU << 4)) | (0xAU << 4);
  GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFFU << 8)) |
                  (1U << 8) | (1U << 12);
  USART2->BRR = (CPU_CLOCK_HZ + 57600U) / 115200U;
  USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void initSysTick(void) {
  SysTick->LOAD = CPU_CLOCK_HZ / 1000U - 1U;
  SysTick->VAL = 0;
  SysTick->CTRL = (1U << 2) | (1U << 1) | (1U << 0);
}

int _write(int file, uint8_t *ptr, int len) {
  (void)file;
  for (int i = 0; i < len; ++i) {
    while (!(USART2->ISR & USART_ISR_TXE)) {
    }
    USART2->TDR = ptr[i];
  }
  return len;
}

static void handleKey(char key) {
  if (key >= '0' && key <= '9') {
    if (state == RESULT) {
      state = FIRST_OPERAND;
    }
    if (state == FIRST_OPERAND) {
      firstOperand = (uint8_t)(key - '0');
      hasFirstOperand = 1;
      operation = 3U;
      hasSecondOperand = 0;
    } else {
      secondOperand = (uint8_t)(key - '0');
      hasSecondOperand = 1;
    }
    tm1637_display_number(key - '0');
    printf("Operand: %c\n", key);
  } else if (key == '*' && state != RESULT) {
    // Выбрать операцию можно только после ввода первого числа.
    // FIRST_OPERAND остаётся состоянием редактирования первого числа.
    if (!hasFirstOperand) return;
    operation = (operation + 1U) % 4U;
    state = SECOND_OPERAND;
    printf("Operation: %c\n", operations[operation]);
  } else if (key == '#' && state == SECOND_OPERAND && hasSecondOperand) {
    int result = 0;
    state = RESULT;
    hasFirstOperand = 0;
    switch (operation) {
      case 0: result = firstOperand + secondOperand; break;
      case 1: result = (int)firstOperand - secondOperand; break;
      case 2: result = firstOperand * secondOperand; break;
      case 3:
        if (secondOperand == 0U) {
          tm1637_display_error();
          printf("Error: division by zero\n");
          return;
        }
        result = firstOperand / secondOperand;
        break;
    }
    tm1637_display_number(result);
    printf("%u %c %u = %d\n", (unsigned)firstOperand,
           operations[operation], (unsigned)secondOperand, result);
  }
}

int main(void) {
  initGPIO();
  initUSART2();
  initKeyboard();
  tm1637_init();
  tm1637_clear();
  printf("Calculator: digit, * select (+ - * /), digit, # calculate.\n");
  initSysTick();
  while (1) {
    char key = scanKeyboard();
    if (key != '\0') handleKey(key);
  }
}
