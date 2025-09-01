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
          -Wall -Wextra -Werror -O2 -std=c11 -ffreestanding -mcmodel=kernel -fno-pie
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

# Stage 3 kernel with interrupts
KERNEL_INT := $(BUILD_DIR)/kernel_interrupts.bin
INTERRUPTS_OBJS := $(BUILD_DIR)/kernel_interrupts.o $(BUILD_DIR)/interrupt_handlers.o $(BUILD_DIR)/isr.o

# Stage 4 kernel with system calls
KERNEL_SYS := $(BUILD_DIR)/kernel_syscalls.bin
SYSCALLS_OBJS := $(BUILD_DIR)/kernel_syscalls.o $(BUILD_DIR)/interrupt_handlers.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall_handlers.o

# Stage 5 kernel with user space
KERNEL_USER := $(BUILD_DIR)/kernel_usermode.bin
USERMODE_OBJS := $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel_usermode.o $(BUILD_DIR)/interrupt_handlers.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/usermode_syscall.o $(BUILD_DIR)/usermode_syscall_handlers.o $(BUILD_DIR)/page_fault_handler.o

# Stage 6 advanced kernel
KERNEL_ADVANCED := $(BUILD_DIR)/kernel_advanced.bin
ADVANCED_OBJS := $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel_advanced.o $(BUILD_DIR)/interrupt_handlers.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/usermode_syscall.o $(BUILD_DIR)/usermode_syscall_handlers.o $(BUILD_DIR)/page_fault_handler.o

# Bootloader target
BOOTLOADER := $(BUILD_DIR)/bootloader.bin

# Kernel target (32-bit protected mode)
KERNEL_PM := $(BUILD_DIR)/kernel_pm.bin

# Legacy kernel target (64-bit multiboot)
KERNEL := $(BUILD_DIR)/kernel.bin

# ISO targets
ISO_PM := $(BUILD_DIR)/tos_pm.iso
ISO := $(BUILD_DIR)/tos.iso

# Default target
all: $(KERNEL_ADVANCED)

# Build bootloader (16-bit real mode)
$(BOOTLOADER): $(SRC_DIR)/bootloader.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f bin -o $@ $<

# Build protected mode kernel (32-bit)
$(KERNEL_PM): $(SRC_DIR)/kernel_pm.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $(BUILD_DIR)/kernel_pm.o
	$(LD) -m elf_i386 -nostdlib -Ttext 0x10000 -o $(BUILD_DIR)/kernel_pm.elf $(BUILD_DIR)/kernel_pm.o
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel_pm.elf $@

# Build kernel with interrupts (32-bit)
$(KERNEL_INT): $(INTERRUPTS_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) -m elf_i386 -nostdlib -Ttext 0x10000 -o $(BUILD_DIR)/kernel_interrupts.elf $(INTERRUPTS_OBJS)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel_interrupts.elf $@

# Build kernel with system calls (32-bit)
$(KERNEL_SYS): $(SYSCALLS_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) -m elf_i386 -nostdlib -Ttext 0x10000 -o $(BUILD_DIR)/kernel_syscalls.elf $(SYSCALLS_OBJS)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel_syscalls.elf $@

# Build kernel with user space (32-bit)
$(KERNEL_USER): $(USERMODE_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) -m elf_i386 -nostdlib -Ttext 0x10000 -o $(BUILD_DIR)/kernel_usermode.elf $(USERMODE_OBJS)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel_usermode.elf $@

# Build advanced kernel (32-bit)
$(KERNEL_ADVANCED): $(ADVANCED_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) -m elf_i386 -nostdlib -Ttext 0x10000 -o $(BUILD_DIR)/kernel_advanced.elf $(ADVANCED_OBJS)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel_advanced.elf $@

# Build legacy kernel (64-bit multiboot)
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

# Compile C files for interrupt kernel
$(BUILD_DIR)/kernel_interrupts.o: $(SRC_DIR)/kernel_interrupts.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/interrupt_handlers.o: $(SRC_DIR)/interrupt_handlers.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/isr.o: $(SRC_DIR)/isr.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/syscall.o: $(SRC_DIR)/syscall.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/kernel_syscalls.o: $(SRC_DIR)/kernel_syscalls.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/syscall_handlers.o: $(SRC_DIR)/syscall_handlers.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/kernel_usermode.o: $(SRC_DIR)/kernel_usermode.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/usermode_syscall.o: $(SRC_DIR)/usermode_syscall.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/usermode_syscall_handlers.o: $(SRC_DIR)/usermode_syscall_handlers.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/page_fault_handler.o: $(SRC_DIR)/page_fault_handler.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/kernel_advanced.o: $(SRC_DIR)/kernel_advanced.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles \
	    -nodefaultlibs -Wall -Wextra -Werror -O2 -std=c11 \
	    -ffreestanding -fno-pie -c $< -o $@

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

# Run protected mode kernel in QEMU with floppy
run-pm: $(BUILD_DIR)/floppy.img
	$(QEMU) -fda $(BUILD_DIR)/floppy.img -monitor stdio

# Run kernel with interrupts in QEMU
run-int: $(BUILD_DIR)/floppy.img
	$(QEMU) -fda $(BUILD_DIR)/floppy.img -monitor stdio

# Run kernel with system calls in QEMU
run-sys: $(BUILD_DIR)/floppy.img
	$(QEMU) -fda $(BUILD_DIR)/floppy.img -monitor stdio

# Run kernel with user space in QEMU
run-user: $(BUILD_DIR)/floppy.img
	$(QEMU) -fda $(BUILD_DIR)/floppy.img -monitor stdio

# Run protected mode ISO in QEMU
run-iso-pm: $(ISO_PM)
	$(QEMU) -cdrom $(ISO_PM) -monitor stdio

# Run in QEMU (legacy multiboot)
run: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -monitor stdio

# Run ISO in QEMU (legacy)
run-iso: $(ISO)
	$(QEMU) -cdrom $(ISO) -monitor stdio

# Debug with GDB
debug: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -s -S -monitor stdio &
	gdb -ex "target remote localhost:1234" -ex "symbol-file $(KERNEL)"

# Create floppy disk image with bootloader and kernel
floppy: $(BOOTLOADER) $(KERNEL_ADVANCED)
	@mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$(BUILD_DIR)/floppy.img bs=512 count=2880
	dd if=$(BOOTLOADER) of=$(BUILD_DIR)/floppy.img bs=512 count=1 conv=notrunc
	dd if=$(KERNEL_ADVANCED) of=$(BUILD_DIR)/floppy.img bs=512 count=60 seek=2 conv=notrunc

# Create bootable ISO for protected mode system
iso-pm: $(ISO_PM)
$(ISO_PM): $(BOOTLOADER) $(KERNEL_PM)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(BOOTLOADER) $(ISO_DIR)/boot/
	cp $(KERNEL_PM) $(ISO_DIR)/boot/kernel.bin
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "Tiny OS (Protected Mode)" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/bootloader.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_PM) $(ISO_DIR)

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
	@echo "  all          - Build kernel with user space (default)"
	@echo "  bootloader   - Build MBR bootloader"
	@echo "  kernel-user  - Build kernel with user space"
	@echo "  kernel-sys   - Build kernel with system calls"
	@echo "  kernel-int   - Build kernel with interrupts"
	@echo "  kernel-pm    - Build protected mode kernel"
	@echo "  kernel       - Build legacy multiboot kernel"
	@echo "  floppy       - Create floppy disk image"
	@echo "  iso          - Create bootable ISO (legacy)"
	@echo "  iso-pm       - Create bootable ISO (protected mode)"
	@echo "  run          - Run kernel in QEMU (legacy)"
	@echo "  run-pm       - Run protected mode kernel in QEMU"
	@echo "  run-int      - Run kernel with interrupts in QEMU"
	@echo "  run-sys      - Run kernel with system calls in QEMU"
	@echo "  run-user     - Run kernel with user space in QEMU"
	@echo "  run-iso      - Run ISO in QEMU (legacy)"
	@echo "  run-iso-pm   - Run protected mode ISO in QEMU"
	@echo "  debug        - Debug with GDB"
	@echo "  clean        - Clean build artifacts"
	@echo "  test-tools   - Test tools availability"
	@echo "  init-dirs    - Create source directory structure"
	@echo "  install-deps - Install system dependencies"
	@echo "  help         - Show this help"

.PHONY: all iso run run-iso run-int run-sys run-user debug clean test-tools init-dirs install-deps help