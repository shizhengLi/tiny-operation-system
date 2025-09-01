# Tiny Operating System Makefile
# Cross-compiler for x86-64 bare metal

# Tool paths
TOOLS_DIR := $(PWD)/tools
PATH := $(TOOLS_DIR)/bin:$(PATH)

# Tools
CC := gcc
LD := ld
ASM := nasm
OBJCOPY := objcopy
QEMU := qemu-system-x86_64

# Flags
CFLAGS := -m64 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
          -Wall -Wextra -Werror -O2 -std=c11 -ffreestanding -mcmodel=kernel
LDFLAGS := -m elf_x86_64 -nostdlib -Ttext 0x100000
ASMFLAGS := -f elf64

# Directories
SRC_DIR := src
BUILD_DIR := build
ISO_DIR := iso

# Source files
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)
ASM_SOURCES := $(wildcard $(SRC_DIR)/*.asm)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
OBJECTS += $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

# Target files
KERNEL := $(BUILD_DIR)/kernel.bin
ISO := $(BUILD_DIR)/tos.iso

# Default target
all: $(KERNEL)

# Build kernel
$(KERNEL): $(OBJECTS) $(SRC_DIR)/linker.ld
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Create bootable ISO
iso: $(ISO)
$(ISO): $(KERNEL)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "Tiny OS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    multiboot2 /boot/kernel.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

# Run in QEMU
run: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -monitor stdio

# Run ISO in QEMU
run-iso: $(ISO)
	$(QEMU) -cdrom $(ISO) -monitor stdio

# Debug with GDB
debug: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -s -S -monitor stdio &
	gdb -ex "target remote localhost:1234" -ex "symbol-file $(KERNEL)"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)

# Install dependencies (requires sudo)
install-deps:
	@echo "Installing dependencies..."
	sudo apt update
	sudo apt install -y build-essential nasm qemu-system-x86_64 gdb

# Test tools availability
test-tools:
	@echo "Testing tools..."
	@which $(CC) > /dev/null && echo "✓ GCC found" || echo "✗ GCC not found"
	@which $(LD) > /dev/null && echo "✓ LD found" || echo "✗ LD not found"
	@which $(ASM) > /dev/null && echo "✓ NASM found" || echo "✗ NASM not found"
	@which $(QEMU) > /dev/null && echo "✓ QEMU found" || echo "✗ QEMU not found"

# Create source directory structure
init-dirs:
	@mkdir -p $(SRC_DIR)
	@echo "Source directory created at $(SRC_DIR)"

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build kernel"
	@echo "  iso          - Create bootable ISO"
	@echo "  run          - Run kernel in QEMU"
	@echo "  run-iso      - Run ISO in QEMU"
	@echo "  debug        - Debug with GDB"
	@echo "  clean        - Clean build artifacts"
	@echo "  test-tools   - Test tools availability"
	@echo "  init-dirs    - Create source directory structure"
	@echo "  install-deps - Install system dependencies"
	@echo "  help         - Show this help"

.PHONY: all iso run run-iso debug clean test-tools init-dirs install-deps help