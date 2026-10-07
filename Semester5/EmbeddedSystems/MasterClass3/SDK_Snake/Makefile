# Set TOOLCHAIN=/path/to/bin/arm-none-eabi- for another compiler.
IDE_GCC := $(firstword $(wildcard /Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*/tools/bin/arm-none-eabi-gcc))
TOOLCHAIN ?= $(if $(IDE_GCC),$(patsubst %gcc,%,$(IDE_GCC)),arm-none-eabi-)
CC := $(TOOLCHAIN)gcc
OBJCOPY := $(TOOLCHAIN)objcopy
SIZE := $(TOOLCHAIN)size
RTOS := Middlewares/Third_Party/FreeRTOS/Source
CPU := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
INCLUDES := -ICore/Inc -IDrivers/STM32F4xx_HAL_Driver/Inc -IDrivers/CMSIS/Include -IDrivers/CMSIS/Device/ST/STM32F4xx/Include -I$(RTOS)/include -I$(RTOS)/CMSIS_RTOS -I$(RTOS)/portable/GCC/ARM_CM4F
CFLAGS := $(CPU) $(INCLUDES) -DSTM32F407xx -DUSE_HAL_DRIVER -std=gnu11 -Og -g3 -Wall -Wextra -ffunction-sections -fdata-sections -MMD -MP
SOURCES := $(wildcard Core/Src/*.c) $(filter-out %_template.c,$(wildcard Drivers/STM32F4xx_HAL_Driver/Src/*.c)) $(wildcard $(RTOS)/*.c) $(RTOS)/CMSIS_RTOS/cmsis_os.c $(RTOS)/portable/GCC/ARM_CM4F/port.c $(RTOS)/portable/MemMang/heap_4.c syscalls.c
OBJECTS := $(addprefix build/,$(SOURCES:.c=.o)) build/startup/startup_stm32f407xx.o
LDFLAGS := $(CPU) -TSTM32F407VGTx_FLASH.ld --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections,-Map=build/SDK_Snake.map -Wl,--start-group -lc -lm -Wl,--end-group
.PHONY: all clean
all: build/SDK_Snake.elf build/SDK_Snake.bin build/SDK_Snake.hex
build/SDK_Snake.elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
build/startup/%.o: startup/%.s
	@mkdir -p $(dir $@)
	$(CC) $(CPU) -c $< -o $@
build/%.bin: build/%.elf
	$(OBJCOPY) -O binary $< $@
build/%.hex: build/%.elf
	$(OBJCOPY) -O ihex $< $@
clean:
	rm -rf build
-include $(OBJECTS:.o=.d)
