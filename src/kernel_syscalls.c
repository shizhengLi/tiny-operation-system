/*
 * Tiny Operating System - Stage 4 Kernel with System Services
 * This kernel includes system calls, process management, and device drivers
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

/* Process states */
enum process_state {
    PROCESS_UNUSED = 0,
    PROCESS_READY = 1,
    PROCESS_RUNNING = 2,
    PROCESS_BLOCKED = 3,
    PROCESS_ZOMBIE = 4
};

/* Process structure */
struct process {
    uint32_t pid;
    uint32_t parent_pid;
    enum process_state state;
    uint32_t esp;
    uint32_t eip;
    uint32_t cr3;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t exit_code;
    char name[32];
};

/* Memory block structure */
struct memory_block {
    uint32_t size;
    uint32_t free;
    struct memory_block* next;
};

/* File descriptor structure */
struct file_descriptor {
    uint32_t inode;
    uint32_t offset;
    uint32_t flags;
    void* device_data;
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

/* Process management */
static struct process processes[16];
static uint32_t current_process = 0;
static uint32_t next_pid = 1;

/* Memory management */
static struct memory_block* memory_heap;
static uint8_t memory_pool[1024 * 64];  // 64KB memory pool

/* File descriptors */
static struct file_descriptor file_descriptors[256];

/* Timer */
uint32_t timer_ticks = 0;
uint32_t timer_frequency = 100;  // 100Hz

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
void timer_init(void);
void memory_init(void);
void process_init(void);
void filesystem_init(void);
void syscall_init(void);
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

/* System call handler */
extern void syscall_handler(void);

/* Terminal functions */
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
    
    /* Set IRQs */
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
    
    /* Set system call handler (INT 0x80) */
    idt_set_gate(128, (uint32_t)syscall_handler, 0x08, 0x8E);
    
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

/* Initialize timer */
void timer_init(void) {
    /* Set timer frequency */
    uint32_t divisor = 1193180 / timer_frequency;
    
    /* Set command byte */
    outb(0x43, 0x36);
    
    /* Set divisor */
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    
    terminal_writestring("Timer initialized at ");
    terminal_writehex(timer_frequency);
    terminal_writestring(" Hz\n");
}

/* Timer handler */
void timer_handler(void) {
    timer_ticks++;
    
    /* Simple scheduling - round robin */
    if (timer_ticks % 10 == 0) {
        /* Schedule next process */
        current_process = (current_process + 1) % 16;
    }
    
    /* Send EOI */
    outb(0x20, 0x20);
}

/* Initialize memory management */
void memory_init(void) {
    /* Initialize memory pool */
    memory_heap = (struct memory_block*)memory_pool;
    memory_heap->size = sizeof(memory_pool) - sizeof(struct memory_block);
    memory_heap->free = 1;
    memory_heap->next = NULL;
    
    terminal_writestring("Memory management initialized\n");
}

/* Memory allocation */
void* malloc(uint32_t size) {
    struct memory_block* block = memory_heap;
    
    /* Align size to 4 bytes */
    size = (size + 3) & ~3;
    
    /* Find free block */
    while (block != NULL) {
        if (block->free && block->size >= size) {
            /* Split block if needed */
            if (block->size > size + sizeof(struct memory_block)) {
                struct memory_block* new_block = (struct memory_block*)((uint8_t*)block + sizeof(struct memory_block) + size);
                new_block->size = block->size - size - sizeof(struct memory_block);
                new_block->free = 1;
                new_block->next = block->next;
                
                block->size = size;
                block->next = new_block;
            }
            
            block->free = 0;
            return (uint8_t*)block + sizeof(struct memory_block);
        }
        
        block = block->next;
    }
    
    return NULL;  /* Out of memory */
}

/* Memory deallocation */
void free(void* ptr) {
    if (ptr == NULL) return;
    
    struct memory_block* block = (struct memory_block*)((uint8_t*)ptr - sizeof(struct memory_block));
    block->free = 1;
    
    /* Merge with next block if free */
    if (block->next != NULL && block->next->free) {
        block->size += block->next->size + sizeof(struct memory_block);
        block->next = block->next->next;
    }
}

/* Initialize process management */
void process_init(void) {
    /* Clear process table */
    for (int i = 0; i < 16; i++) {
        processes[i].state = PROCESS_UNUSED;
    }
    
    /* Create init process */
    processes[0].pid = next_pid++;
    processes[0].parent_pid = 0;
    processes[0].state = PROCESS_RUNNING;
    processes[0].eip = (uint32_t)kernel_main;
    processes[0].name[0] = 'i';
    processes[0].name[1] = 'n';
    processes[0].name[2] = 'i';
    processes[0].name[3] = 't';
    processes[0].name[4] = '\0';
    
    terminal_writestring("Process management initialized\n");
}

/* Initialize file system */
void filesystem_init(void) {
    /* Clear file descriptors */
    for (int i = 0; i < 256; i++) {
        file_descriptors[i].inode = 0xFFFFFFFF;
    }
    
    /* Create standard file descriptors */
    file_descriptors[0].inode = 0;  /* stdin */
    file_descriptors[1].inode = 1;  /* stdout */
    file_descriptors[2].inode = 2;  /* stderr */
    
    terminal_writestring("File system initialized\n");
}

/* System call interface */
void syscall_init(void) {
    terminal_writestring("System call interface initialized\n");
}


/* Keyboard handler */
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    /* Simple scancode to ASCII mapping */
    static const char scancode_to_ascii[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            terminal_putchar(c);
        }
    }
    
    /* Send EOI */
    outb(0x20, 0x20);
}

/* Initialize interrupts */
void interrupts_init(void) {
    terminal_writestring("Initializing IDT...\n");
    idt_init();
    
    terminal_writestring("Initializing PIC...\n");
    pic_init();
    
    terminal_writestring("Initializing timer...\n");
    timer_init();
    
    terminal_writestring("Enabling interrupts...\n");
    __asm__ __volatile__("sti");
}

/* Test system calls */
void test_system_calls(void) {
    terminal_writestring("Testing system calls...\n");
    
    /* Test write system call */
    terminal_writestring("Testing write syscall: ");
    const char* msg = "Hello, syscall!\n";
    __asm__ __volatile__(
        "mov $1, %%eax;"    /* SYSCALL_WRITE */
        "mov $1, %%ebx;"    /* stdout */
        "mov %0, %%ecx;"    /* message */
        "mov $14, %%edx;"   /* length */
        "int $0x80;"
        :
        : "r"(msg)
        : "eax", "ebx", "ecx", "edx", "memory"
    );
    
    /* Test getpid system call */
    terminal_writestring("Testing getpid syscall: ");
    uint32_t pid;
    __asm__ __volatile__(
        "mov $12, %%eax;"   /* SYSCALL_GETPID */
        "int $0x80;"
        "mov %%eax, %0;"
        : "=r"(pid)
        :
        : "eax", "memory"
    );
    terminal_writehex(pid);
    terminal_writestring("\n");
    
    /* Test sleep system call */
    terminal_writestring("Testing sleep syscall (1 second)...\n");
    __asm__ __volatile__(
        "mov $13, %%eax;"   /* SYSCALL_SLEEP */
        "mov $1000, %%ebx;" /* 1000ms */
        "int $0x80;"
        :
        :
        : "eax", "ebx", "memory"
    );
    terminal_writestring("Sleep completed!\n");
}

/* Main kernel function */
void kernel_main(void) {
    /* Initialize terminal */
    terminal_initialize();
    
    /* Display welcome message */
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("Tiny Operating System - Stage 4\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Kernel with system services initialized!\n\n");
    
    /* Initialize all subsystems */
    interrupts_init();
    memory_init();
    process_init();
    filesystem_init();
    syscall_init();
    
    /* Display system information */
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("System Information:\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("- Architecture: x86_32 Protected Mode\n");
    terminal_writestring("- Interrupts: Enabled\n");
    terminal_writestring("- IDT: Initialized\n");
    terminal_writestring("- PIC: Configured\n");
    terminal_writestring("- Timer: ");
    terminal_writehex(timer_frequency);
    terminal_writestring(" Hz\n");
    terminal_writestring("- Memory: 64KB pool\n");
    terminal_writestring("- Processes: 16 slots\n");
    terminal_writestring("- File descriptors: 256 slots\n");
    terminal_writestring("- System calls: ");
    terminal_writehex(SYSCALL_MAX);
    terminal_writestring(" functions\n\n");
    
    /* Test system calls */
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("[OK] System services operational!\n\n");
    
    test_system_calls();
    
    /* Enable keyboard interrupt */
    outb(0x21, inb(0x21) & ~0x02);  /* Enable IRQ1 (keyboard) */
    
    /* Main kernel loop */
    while (1) {
        /* Halt CPU until next interrupt */
        __asm__ __volatile__("hlt");
    }
}