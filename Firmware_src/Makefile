# Disable built‐in suffix rules to avoid target collisions.
.SUFFIXES:

#------------------------------------------------------------------------------
# Toolchain Settings
#------------------------------------------------------------------------------
CC      := arm-none-eabi-gcc       # GCC for ARM Cortex-M0
AS      := arm-none-eabi-gcc       # Use GCC as assembler (with CPP)
OBJCOPY := arm-none-eabi-objcopy
PYTHON  := python3                 # Python 3 interpreter

# Use a target base name that does not conflict with any folder names.
TARGET  := prox_reporter_app

#------------------------------------------------------------------------------
# Processor Flags for DA14585 (Cortex-M0)
#------------------------------------------------------------------------------
MCU     := -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
CFLAGS  := $(MCU) -Os -g -ffunction-sections -fdata-sections -Wall -Wextra -MMD -MP
ASFLAGS := $(MCU) -x assembler-with-cpp
LDFLAGS := $(MCU) -Wl,--gc-sections -Wl,-Map=$(TARGET).map

#------------------------------------------------------------------------------
# Dynamic Include Directories Detection
#------------------------------------------------------------------------------
# Automatically find all directories containing header files (*.h) and add to include paths
HEADER_DIRS := $(shell find . -type f -name '*.h' -exec dirname {} \; | sort -u)
CFLAGS += $(addprefix -I, $(HEADER_DIRS))
LDS_INC := $(addprefix -I, $(HEADER_DIRS))

#------------------------------------------------------------------------------
# Directories and Source File Lists
#------------------------------------------------------------------------------
PROJECT_DIR  := projects/target_apps/ble_examples/prox_reporter
SDK_DIR      := sdk
ECLIPSE_DIR  := $(PROJECT_DIR)/Eclipse
BUILD_DIR    := build
BIN_DIR      := $(BUILD_DIR)
OBJ_DIR      := $(BUILD_DIR)/obj

# Gather all C source files from project and SDK directories.
SRC_C := \
	$(shell find $(PROJECT_DIR)/src -type f -name '*.c') \
	$(shell find $(SDK_DIR)/app_modules/src -type f -name '*.c') \
	$(shell find $(SDK_DIR)/ble_stack -type f -name '*.c') \
	$(shell find $(SDK_DIR)/platform -type f -name '*.c') \
	$(shell find third_party -type f -name '*.c')

# Gather assembly sources (e.g., startup files).
SRC_ASM := $(shell find $(SDK_DIR)/platform/arch/boot/GCC -type f -name '*.S')

# Convert source file names to object file paths.
OBJS  := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_C))
OBJS += $(patsubst %.S,$(OBJ_DIR)/%.o,$(SRC_ASM))
DEPS := $(OBJS:.o=.d)

#------------------------------------------------------------------------------
# Output File Names
#------------------------------------------------------------------------------
ELF     := $(BIN_DIR)/$(TARGET).elf
BIN     := $(BIN_DIR)/$(TARGET).bin
OTA_BIN := $(BIN_DIR)/$(TARGET)_OTA.bin

#------------------------------------------------------------------------------
# Linker Script Preprocessing
#------------------------------------------------------------------------------
# Preprocess the linker script template so that #include directives
# (like including da1458x_config_basic.h) find the necessary headers.
LD_SCRIPT_SRC := sdk/common_project_files/ldscripts/ldscript_DA14585_586.lds.S
LD_SCRIPT     := $(OBJ_DIR)/da14585_586.ld

#------------------------------------------------------------------------------
# Build Targets
#------------------------------------------------------------------------------
.PHONY: all clean
all: $(OTA_BIN)

$(LD_SCRIPT): $(LD_SCRIPT_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDS_INC) -E -P -x assembler-with-cpp $< -o $@

# Compile C files to object files.
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S files to object files.
$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Link: create the ELF file using the preprocessed linker script.
$(ELF): $(OBJS) $(LD_SCRIPT)
	$(CC) $(CFLAGS) $(LDFLAGS) -T$(LD_SCRIPT) -o $@ $(OBJS)
	@echo "Linked: $(ELF)"

# Convert the ELF file to a raw binary.
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Generated raw binary: $(BIN)"

# Generate the OTA-ready image by invoking the provided Python script.
$(OTA_BIN): $(BIN) $(ECLIPSE_DIR)/make_bin_ota.py
	$(PYTHON) $(ECLIPSE_DIR)/make_bin_ota.py $< $@
	@echo "OTA image generated: $(OTA_BIN)"

# Clean up build artifacts.
clean:
	rm -rf $(BUILD_DIR) $(TARGET).map

# Include dependency files.
-include $(DEPS)

