# STM32F407G-DISC1 smart home control hub with FreeRTOS
#
# Needs: arm-none-eabi-gcc, make and the STM32CubeF4 package (HAL, CMSIS, FreeRTOS, fonts).
#   make                 builds the firmware (build/firmware.elf, .bin, .hex)
#   make FW=/path/STM32Cube_FW_F4_V1.28.3     if the package is somewhere else
#   make test            runs the hardware-free logic tests on the PC
#   make flash           flashes over ST-LINK (needs STM32_Programmer_CLI)

FW ?= $(subst \,/,$(USERPROFILE))/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3

TARGET  := firmware
BUILD   := build
CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

HAL     := $(FW)/Drivers/STM32F4xx_HAL_Driver
CMSIS   := $(FW)/Drivers/CMSIS
RTOS    := $(FW)/Middlewares/Third_Party/FreeRTOS/Source

HAL_SRC := hal hal_cortex hal_rcc hal_rcc_ex hal_gpio hal_dma hal_dma_ex hal_pwr hal_pwr_ex \
           hal_flash hal_flash_ex hal_adc hal_adc_ex hal_i2c hal_i2c_ex hal_tim hal_tim_ex hal_uart
SRCS := $(wildcard src/*.c) \
        $(foreach m,$(HAL_SRC),$(HAL)/Src/stm32f4xx_$(m).c) \
        $(CMSIS)/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
        $(RTOS)/tasks.c $(RTOS)/queue.c $(RTOS)/list.c \
        $(RTOS)/portable/GCC/ARM_CM4F/port.c $(RTOS)/portable/MemMang/heap_4.c \
        $(FW)/Utilities/Fonts/font8.c
ASRC := $(CMSIS)/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s

OBJS := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SRCS))) $(patsubst %.s,$(BUILD)/%.o,$(notdir $(ASRC)))
VPATH := $(sort $(dir $(SRCS) $(ASRC)))

MCU    := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
DEFS   := -DUSE_HAL_DRIVER -DSTM32F407xx
INCS   := -Iinc -I$(HAL)/Inc -I$(CMSIS)/Device/ST/STM32F4xx/Include -I$(CMSIS)/Include \
          -I$(RTOS)/include -I$(RTOS)/portable/GCC/ARM_CM4F -I$(FW)/Utilities/Fonts
CFLAGS := $(MCU) $(DEFS) $(INCS) -Os -g3 -std=gnu11 -Wall -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := $(MCU) -Tstm32f407_flash.ld -specs=nano.specs -specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -Wl,--print-memory-usage

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	$(CC) $(MCU) -x assembler-with-cpp -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD):
	mkdir -p $@

test:
	gcc -Wall -Wextra -Iinc tests/test_logic.c src/app_logic.c -o $(BUILD)/test_logic
	$(BUILD)/test_logic

flash: $(BUILD)/$(TARGET).elf
	STM32_Programmer_CLI -c port=SWD -w $< -v -rst

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
.PHONY: all test flash clean
