/*
 * Tiny Operating System - Interrupt Handlers
 * C functions for handling interrupts and exceptions
 */

#include <stdint.h>
#include <stddef.h>

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
    VGA_COLOR_LIGHT_YELLOW = 14,
};

/* Exception messages */
static const char* exception_messages[] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Into detected overflow",
    "Out of bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "SIMD floating point exception",
    "Virtualization exception",
    "Security exception"
};

/* Terminal functions */
static void terminal_setcolor(enum vga_color color);
static void terminal_putchar(char c);
static void terminal_writestring(const char* data);

/* Set terminal color */
static void terminal_setcolor(enum vga_color color) {
    /* This function is simplified for interrupt handlers */
    (void)color; /* Suppress unused parameter warning */
}

/* Put a character */
static void terminal_putchar(char c) {
    volatile uint16_t* terminal_buffer = VGA_BUFFER;
    static size_t terminal_row = 0;
    static size_t terminal_column = 0;
    static enum vga_color terminal_color = VGA_COLOR_LIGHT_GREY;
    
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == 25) {
            terminal_row = 0;
        }
        return;
    }
    
    const size_t index = terminal_row * 80 + terminal_column;
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

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* External interrupt handlers */
extern void keyboard_handler(void);
extern void timer_handler(void);

/* ISR handler function */
void isr_handler(uint32_t interrupt_number, uint32_t error_code) {
    terminal_setcolor(VGA_COLOR_LIGHT_RED);
    terminal_writestring("EXCEPTION: ");
    
    if (interrupt_number < sizeof(exception_messages) / sizeof(exception_messages[0])) {
        terminal_writestring(exception_messages[interrupt_number]);
    } else {
        terminal_writestring("Unknown exception");
    }
    
    terminal_writestring(" (");
    terminal_writehex(interrupt_number);
    terminal_writestring(")");
    
    if (error_code != 0) {
        terminal_writestring(" Error code: ");
        terminal_writehex(error_code);
    }
    
    terminal_writestring("\nSystem halted.\n");
    
    /* Halt the system */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

/* IRQ handler function */
void irq_handler(uint32_t irq_number) {
    /* Handle specific IRQs */
    switch (irq_number) {
        case 32:  /* Timer (IRQ 0) */
            timer_handler();
            break;
        case 33:  /* Keyboard (IRQ 1) */
            keyboard_handler();
            break;
        default:
            /* Unhandled IRQ */
            terminal_setcolor(VGA_COLOR_LIGHT_YELLOW);
            terminal_writestring("Unhandled IRQ: ");
            terminal_writehex(irq_number);
            terminal_writestring("\n");
            break;
    }
    
    /* Send EOI to PIC */
    if (irq_number >= 40) {
        /* Send EOI to slave PIC */
        outb(0xA0, 0x20);
    }
    /* Send EOI to master PIC */
    outb(0x20, 0x20);
}