#include "main.h"
#include "keyboard.h"
#include "tm1637.h"

#define LED_PIN 5U
#define MAX_SECONDS 9999U

typedef enum {
  TIMER_INPUT,
  TIMER_RUNNING,
  TIMER_FINISHED
} TimerState;

volatile uint32_t tickCount = 0;
static TimerState timerState = TIMER_INPUT;
static uint16_t seconds = 0;
static uint8_t inputDigits = 0;
static uint32_t lastSecondTime = 0;

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

static void resetTimer(void) {
  timerState = TIMER_INPUT;
  seconds = 0;
  inputDigits = 0;
  setLED(0);
  tm1637_display_number(seconds);
}

static void finishTimer(void) {
  timerState = TIMER_FINISHED;
  seconds = 0;
  setLED(1);
  tm1637_display_number(seconds);
  printf("Timer finished!\n");
}

static void handleKey(char key) {
  if (key == '*') {
    resetTimer();
    return;
  }

  // Во время отсчёта доступен только сброс.
  if (timerState == TIMER_RUNNING) {
    return;
  }

  if (key >= '0' && key <= '9') {
    if (timerState == TIMER_FINISHED) {
      resetTimer();
    }
    if (inputDigits < 4U) {
      uint32_t value = (uint32_t)seconds * 10U + (uint32_t)(key - '0');
      if (value <= MAX_SECONDS) {
        seconds = (uint16_t)value;
        ++inputDigits;
        tm1637_display_number(seconds);
      }
    }
  } else if (key == '#' && timerState == TIMER_INPUT) {
    setLED(0);
    if (seconds == 0U) {
      finishTimer();
    } else {
      lastSecondTime = tickCount;
      timerState = TIMER_RUNNING;
      printf("Timer started: %u s\n", (unsigned)seconds);
    }
  }
}

static void updateTimer(void) {
  if (timerState != TIMER_RUNNING) {
    return;
  }

  uint32_t now = tickCount;
  // Беззнаковое вычитание работает и при переполнении tickCount.
  uint32_t elapsedSeconds = (uint32_t)(now - lastSecondTime) / 1000U;
  if (elapsedSeconds == 0U) {
    return;
  }

  if (elapsedSeconds >= seconds) {
    finishTimer();
  } else {
    seconds -= (uint16_t)elapsedSeconds;
    // Сохраняем остаток миллисекунд, чтобы не накапливать задержку.
    lastSecondTime += elapsedSeconds * 1000U;
    tm1637_display_number(seconds);
  }
}

int main(void) {
  initGPIO();
  initUSART2();
  initKeyboard();
  tm1637_init();
  resetTimer();
  printf("Enter seconds (0-9999), # start, * reset.\n");
  initSysTick();

  while (1) {
    updateTimer();
    char key = scanKeyboard();
    if (key != '\0') {
      handleKey(key);
    }
  }
}
