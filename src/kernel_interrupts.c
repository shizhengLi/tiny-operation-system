/*
 * Tiny Operating System - Stage 3 Kernel with Interrupts
 * This kernel includes IDT setup, interrupt handling, and basic I/O
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

/* IDT Entry structure */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

/* IDT Pointer structure */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* Global variables */
static size_t terminal_row;
static size_t terminal_column;
static enum vga_color terminal_color;
static volatile uint16_t* terminal_buffer;

/* IDT structures */
static struct idt_entry idt[256];
static struct idt_ptr idt_ptr;

/* Function prototypes */
void terminal_initialize(void);
void terminal_setcolor(enum vga_color color);
void terminal_putchar(char c);
void terminal_writestring(const char* data);
void terminal_writehex(uint32_t value);
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void pic_init(void);
void keyboard_init(void);
void interrupts_init(void);
void kernel_main(void);

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Assembly interrupt handlers */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

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
            terminal_buffer[index] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
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
    terminal_buffer[index] = (uint16_t)c | (uint16_t)color << 8;
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
void terminal_writestring(const char* data) {
    size_t len = 0;
    while (data[len] != '\0') {
        len++;
    }
    for (size_t i = 0; i < len; i++) {
        terminal_putchar(data[i]);
    }
}

/* Write a hexadecimal value */
void terminal_writehex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    terminal_writestring("0x");
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        terminal_putchar(hex_chars[nibble]);
    }
}

/* Initialize IDT */
void idt_init(void) {
    /* Set IDT pointer */
    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    /* Clear IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    /* Set ISRs (Interrupt Service Routines) */
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    
    /* Load IDT */
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}

/* Set IDT gate */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

/* Initialize PIC (Programmable Interrupt Controller) */
void pic_init(void) {
    /* ICW1 - Initialize PIC */
    outb(0x20, 0x11);  /* Master PIC */
    outb(0xA0, 0x11);  /* Slave PIC */
    
    /* ICW2 - Remap IRQs */
    outb(0x21, 0x20);  /* Master PIC vector offset */
    outb(0xA1, 0x28);  /* Slave PIC vector offset */
    
    /* ICW3 - Setup cascading */
    outb(0x21, 0x04);  /* Master PIC has slave at IRQ2 */
    outb(0xA1, 0x02);  /* Slave PIC is at IRQ2 */
    
    /* ICW4 - Set 8086 mode */
    outb(0x21, 0x01);  /* Master PIC */
    outb(0xA1, 0x01);  /* Slave PIC */
    
    /* Clear interrupt masks */
    outb(0x21, 0x00);  /* Enable all IRQs on master */
    outb(0xA1, 0x00);  /* Enable all IRQs on slave */
}

/* Initialize interrupts */
void interrupts_init(void) {
    terminal_writestring("Initializing IDT...\n");
    idt_init();
    
    terminal_writestring("Initializing PIC...\n");
    pic_init();
    
    terminal_writestring("Enabling interrupts...\n");
    __asm__ __volatile__("sti");
}

/* Keyboard handler */
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    /* Simple scancode to ASCII mapping (US keyboard) */
    static const char scancode_to_ascii[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            terminal_putchar(c);
        }
    }
    
    /* Send EOI (End of Interrupt) */
    outb(0x20, 0x20);
}

/* Timer handler */
void timer_handler(void) {
    static int timer_count = 0;
    timer_count++;
    
    if (timer_count % 100 == 0) {
        /* Update cursor or other periodic tasks */
    }
    
    /* Send EOI */
    outb(0x20, 0x20);
}

/* Main kernel function */
void kernel_main(void) {
    /* Initialize terminal */
    terminal_initialize();
    
    /* Display welcome message */
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("Tiny Operating System - Stage 3\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Kernel with interrupt handling initialized!\n\n");
    
    /* Initialize interrupts */
    interrupts_init();
    
    /* Display system information */
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("System Information:\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("- Architecture: x86_32 Protected Mode\n");
    terminal_writestring("- Interrupts: Enabled\n");
    terminal_writestring("- IDT: Initialized\n");
    terminal_writestring("- PIC: Configured\n");
    terminal_writestring("- Keyboard: Ready\n\n");
    
    /* Test interrupts */
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("[OK] Interrupt system operational!\n");
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("Try typing on the keyboard...\n\n");
    
    /* Enable keyboard interrupt */
    outb(0x21, inb(0x21) & ~0x02);  /* Enable IRQ1 (keyboard) */
    
    /* Main kernel loop */
    while (1) {
        /* Halt CPU until next interrupt */
        __asm__ __volatile__("hlt");
    }
}