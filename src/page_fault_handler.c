/*
 * Tiny Operating System - Page Fault Handler
 * C functions for handling page faults
 */

#include <stdint.h>

/* VGA text mode constants */
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

/* Page fault error codes */
#define PF_PRESENT    0x01
#define PF_WRITE      0x02
#define PF_USER       0x04
#define PF_RESERVED   0x08
#define PF_INSTRUCTION 0x10

/* Terminal functions */
static void terminal_putchar(char c);
static void terminal_writestring(const char* data);
static void terminal_writehex(uint32_t value);

/* External functions */
void process_kill(uint32_t pid);

/* Put a character */
static void terminal_putchar(char c) {
    volatile uint16_t* terminal_buffer = VGA_BUFFER;
    static uint32_t terminal_row = 0;
    static uint32_t terminal_column = 0;
    static enum vga_color terminal_color = VGA_COLOR_LIGHT_GREY;
    
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == 25) {
            terminal_row = 0;
        }
        return;
    }
    
    const uint32_t index = terminal_row * 80 + terminal_column;
    terminal_buffer[index] = (uint16_t)c | (uint16_t)terminal_color << 8;
    
    if (++terminal_column == 80) {
        terminal_column = 0;
        if (++terminal_row == 25) {
            terminal_row = 0;
        }
    }
}

/* Write a string */
static void terminal_writestring(const char* data) {
    while (*data != '\0') {
        terminal_putchar(*data);
        data++;
    }
}

/* Write a hexadecimal value */
static void terminal_writehex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    terminal_writestring("0x");
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        terminal_putchar(hex_chars[nibble]);
    }
}

/* Page fault handler */
void page_fault_handler_c(uint32_t faulting_address, uint32_t error_code) {
    terminal_writestring("PAGE FAULT!\n");
    terminal_writestring("Faulting address: ");
    terminal_writehex(faulting_address);
    terminal_writestring("\n");
    
    terminal_writestring("Error code: ");
    terminal_writehex(error_code);
    terminal_writestring(" (");
    
    if (error_code & PF_PRESENT) {
        terminal_writestring("PRESENT ");
    }
    if (error_code & PF_WRITE) {
        terminal_writestring("WRITE ");
    }
    if (error_code & PF_USER) {
        terminal_writestring("USER ");
    }
    if (error_code & PF_RESERVED) {
        terminal_writestring("RESERVED ");
    }
    if (error_code & PF_INSTRUCTION) {
        terminal_writestring("INSTRUCTION ");
    }
    terminal_writestring(")\n");
    
    /* Check if fault occurred in user space */
    if (error_code & PF_USER) {
        terminal_writestring("User space page fault - killing process\n");
        
        /* Kill the current process */
        extern uint32_t current_process;
        process_kill(current_process);
        
        /* This should trigger a context switch */
        while (1) {
            /* Halt until interrupt */
            __asm__ __volatile__("hlt");
        }
    } else {
        terminal_writestring("Kernel space page fault - system halted\n");
        while (1) {
            /* Halt system */
            __asm__ __volatile__("hlt");
        }
    }
}