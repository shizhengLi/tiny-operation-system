/*
 * Tiny Operating System - System Call Handlers
 * C functions for handling system calls
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

/* System call numbers */
enum syscall_num {
    SYSCALL_EXIT = 0,
    SYSCALL_READ = 1,
    SYSCALL_WRITE = 2,
    SYSCALL_OPEN = 3,
    SYSCALL_CLOSE = 4,
    SYSCALL_SEEK = 5,
    SYSCALL_MMAP = 6,
    SYSCALL_MUNMAP = 7,
    SYSCALL_FORK = 8,
    SYSCALL_EXEC = 9,
    SYSCALL_WAIT = 10,
    SYSCALL_KILL = 11,
    SYSCALL_GETPID = 12,
    SYSCALL_SLEEP = 13,
    SYSCALL_YIELD = 14,
    SYSCALL_MAX = 15
};

/* Terminal functions */
static void terminal_putchar(char c);
static void terminal_writestring(const char* data);
static void terminal_writehex(uint32_t value);

/* External timer variable */
extern uint32_t timer_ticks;
extern uint32_t timer_frequency;

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

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

/* System call handler */
void syscall_handler_c(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    /* Suppress unused parameter warnings */
    (void)arg4;
    (void)arg5;
    
    switch (syscall_num) {
        case SYSCALL_EXIT:
            terminal_writestring("Process exited with code ");
            terminal_writehex(arg1);
            terminal_writestring("\n");
            break;
            
        case SYSCALL_WRITE:
            if (arg1 == 1 || arg1 == 2) {  /* stdout or stderr */
                const char* buf = (const char*)arg2;
                uint32_t count = arg3;
                for (uint32_t i = 0; i < count; i++) {
                    terminal_putchar(buf[i]);
                }
            }
            break;
            
        case SYSCALL_READ:
            /* Simple read implementation */
            if (arg1 == 0) {  /* stdin */
                /* Return 0 for now (no input available) */
                return;
            }
            break;
            
        case SYSCALL_GETPID:
            /* Return current process PID */
            __asm__ __volatile__("mov %0, %%eax" : : "r"(1));  /* Simplified */
            return;
            
        case SYSCALL_SLEEP:
            /* Simple sleep implementation */
            if (timer_frequency > 0) {
                uint32_t sleep_ticks = arg1 * timer_frequency / 1000;  /* Convert ms to ticks */
                uint32_t start_ticks = timer_ticks;
                while (timer_ticks - start_ticks < sleep_ticks) {
                    __asm__ __volatile__("hlt");
                }
            }
            break;
            
        default:
            terminal_writestring("Unknown system call: ");
            terminal_writehex(syscall_num);
            terminal_writestring("\n");
            break;
    }
}