################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/fonts.c \
../Src/gpio.c \
../Src/i2c.c \
../Src/kb.c \
../Src/main.c \
../Src/oled.c \
../Src/pca9538.c \
../Src/sdk_uart.c \
../Src/stm32f4xx_hal_msp.c \
../Src/stm32f4xx_it.c \
../Src/syscalls.c \
../Src/system_stm32f4xx.c \
../Src/usart.c 

OBJS += \
./Src/fonts.o \
./Src/gpio.o \
./Src/i2c.o \
./Src/kb.o \
./Src/main.o \
./Src/oled.o \
./Src/pca9538.o \
./Src/sdk_uart.o \
./Src/stm32f4xx_hal_msp.o \
./Src/stm32f4xx_it.o \
./Src/syscalls.o \
./Src/system_stm32f4xx.o \
./Src/usart.o 

C_DEPS += \
./Src/fonts.d \
./Src/gpio.d \
./Src/i2c.d \
./Src/kb.d \
./Src/main.d \
./Src/oled.d \
./Src/pca9538.d \
./Src/sdk_uart.d \
./Src/stm32f4xx_hal_msp.d \
./Src/stm32f4xx_it.d \
./Src/syscalls.d \
./Src/system_stm32f4xx.d \
./Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Og -ffunction-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/fonts.cyclo ./Src/fonts.d ./Src/fonts.o ./Src/fonts.su ./Src/gpio.cyclo ./Src/gpio.d ./Src/gpio.o ./Src/gpio.su ./Src/i2c.cyclo ./Src/i2c.d ./Src/i2c.o ./Src/i2c.su ./Src/kb.cyclo ./Src/kb.d ./Src/kb.o ./Src/kb.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/oled.cyclo ./Src/oled.d ./Src/oled.o ./Src/oled.su ./Src/pca9538.cyclo ./Src/pca9538.d ./Src/pca9538.o ./Src/pca9538.su ./Src/sdk_uart.cyclo ./Src/sdk_uart.d ./Src/sdk_uart.o ./Src/sdk_uart.su ./Src/stm32f4xx_hal_msp.cyclo ./Src/stm32f4xx_hal_msp.d ./Src/stm32f4xx_hal_msp.o ./Src/stm32f4xx_hal_msp.su ./Src/stm32f4xx_it.cyclo ./Src/stm32f4xx_it.d ./Src/stm32f4xx_it.o ./Src/stm32f4xx_it.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/system_stm32f4xx.cyclo ./Src/system_stm32f4xx.d ./Src/system_stm32f4xx.o ./Src/system_stm32f4xx.su ./Src/usart.cyclo ./Src/usart.d ./Src/usart.o ./Src/usart.su

.PHONY: clean-Src

