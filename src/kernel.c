/*
 * Tiny Operating System - Hello World Kernel Stub
 * This is the minimal kernel entry point for stage 1
 */

#include <stddef.h>
#include <stdint.h>

/* VGA text mode constants */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((volatile uint16_t*)0xB8000)

/* VGA colors */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

/* VGA entry structure */
static inline uint16_t vga_entry(unsigned char uc, enum vga_color color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

/* Global variables */
static size_t terminal_row;
static size_t terminal_column;
static enum vga_color terminal_color;
static volatile uint16_t* terminal_buffer;

/* Terminal initialization */
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = VGA_COLOR_LIGHT_GREY;
    terminal_buffer = VGA_BUFFER;
    
    /* Clear the screen */
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

/* Set terminal color */
void terminal_setcolor(enum vga_color color) {
    terminal_color = color;
}

/* Put a character at position */
void terminal_putentryat(char c, enum vga_color color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

/* Put a character */
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

/* Write a string */
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

/* Write a string (null-terminated) */
void terminal_writestring(const char* data) {
    size_t len = 0;
    while (data[len] != '\0') {
        len++;
    }
    terminal_write(data, len);
}

/* Kernel main function */
void kernel_main(void) {
    /* Initialize terminal */
    terminal_initialize();
    
    /* Display welcome message */
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("Tiny Operating System - Stage 1\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Hello World! Kernel loaded successfully.\n");
    
    /* Display system info */
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("System Information:\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("- Architecture: x86_64\n");
    terminal_writestring("- Target: QEMU Emulator\n");
    terminal_writestring("- Stage: 1 - Development Environment Setup\n");
    
    /* Success indicator */
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("\n[OK] All systems operational!\n");
    
    /* Infinite loop to prevent kernel exit */
    while (1) {
        // CPU halt instruction to reduce power consumption
        __asm__ __volatile__("hlt");
    }
}