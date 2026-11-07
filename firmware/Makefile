##########################################################################
# firmware/Makefile — GNU make для STM32C031C6 (Nucleo-C031C6 / Wokwi)
# Структура як у CubeMX-проєкту з Toolchain = Makefile.
# Викликається з кореневого Makefile всередині Docker-контейнера.
##########################################################################
TARGET    = firmware
BUILD_DIR = build
DEBUG    ?= 1
OPT       = -Og

C_SOURCES = \
Core/Src/main.c \
Core/Src/stm32c0xx_it.c \
Core/Src/stm32c0xx_hal_msp.c \
Core/Src/system_stm32c0xx.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c \
$(wildcard Drivers/STM32C0xx_HAL_Driver/Src/*.c)

ASM_SOURCES = startup_stm32c031xx.s

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

# Cortex-M0+ без FPU (у Nucleo-F411/F446 тут було б -mcpu=cortex-m4
# -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
CPU = -mcpu=cortex-m0plus
MCU = $(CPU) -mthumb

C_DEFS = -DUSE_HAL_DRIVER -DSTM32C031xx
C_INCLUDES = \
-ICore/Inc \
-IDrivers/STM32C0xx_HAL_Driver/Inc \
-IDrivers/STM32C0xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32C0xx/Include \
-IDrivers/CMSIS/Include

ASFLAGS = $(MCU) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS  = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Відтворювані збірки: без шляхів хоста та часових міток
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -ffile-prefix-map=$(CURDIR)=.

LDSCRIPT = STM32C031C6Tx_FLASH.ld
LIBS     = -lc -lm -lnosys
LDFLAGS  = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections \
           -Wl,--no-warn-rwx-segments

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "AS  $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "LD  $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@$(CP) -O binary -S $< $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
-include $(wildcard $(BUILD_DIR)/*.d)
