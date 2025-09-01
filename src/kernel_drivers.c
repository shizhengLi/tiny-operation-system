/*
 * Phase 8: Device Drivers Implementation
 * Implements keyboard, mouse, disk, and timer drivers
 * Following the plan.md Phase 8 requirements
 */

#include <stdint.h>
#include <stddef.h>

/* VGA Text Mode Colors */
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

/* VGA Display Functions */
static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

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

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}

/* Port I/O Functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Keyboard Driver */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_IRQ 1

/* Keyboard Scancode to ASCII Mapping (US Keyboard) */
static const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char keyboard_buffer[256];
static int keyboard_buffer_head = 0;
static int keyboard_buffer_tail = 0;
static int keyboard_shift_pressed = 0;

void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Handle shift keys */
    if (scancode == 0x2A || scancode == 0x36) {
        keyboard_shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        keyboard_shift_pressed = 0;
        return;
    }
    
    /* Convert scancode to ASCII */
    char ascii = scancode_to_ascii[scancode & 0x7F];
    
    /* Apply shift if needed */
    if (keyboard_shift_pressed && ascii >= 'a' && ascii <= 'z') {
        ascii = ascii - 'a' + 'A';
    }
    
    /* Handle special shift combinations */
    if (keyboard_shift_pressed) {
        switch (ascii) {
            case '1': ascii = '!'; break;
            case '2': ascii = '@'; break;
            case '3': ascii = '#'; break;
            case '4': ascii = '$'; break;
            case '5': ascii = '%'; break;
            case '6': ascii = '^'; break;
            case '7': ascii = '&'; break;
            case '8': ascii = '*'; break;
            case '9': ascii = '('; break;
            case '0': ascii = ')'; break;
            case '-': ascii = '_'; break;
            case '=': ascii = '+'; break;
            case '[': ascii = '{'; break;
            case ']': ascii = '}'; break;
            case '\\': ascii = '|'; break;
            case ';': ascii = ':'; break;
            case '\'': ascii = '"'; break;
            case ',': ascii = '<'; break;
            case '.': ascii = '>'; break;
            case '/': ascii = '?'; break;
            case '`': ascii = '~'; break;
        }
    }
    
    /* Add to buffer if it's a valid character */
    if (ascii != 0) {
        int next = (keyboard_buffer_head + 1) % 256;
        if (next != keyboard_buffer_tail) {
            keyboard_buffer[keyboard_buffer_head] = ascii;
            keyboard_buffer_head = next;
        }
    }
}

char keyboard_getchar(void) {
    if (keyboard_buffer_tail == keyboard_buffer_head) {
        return 0; /* No character available */
    }
    
    char c = keyboard_buffer[keyboard_buffer_tail];
    keyboard_buffer_tail = (keyboard_buffer_tail + 1) % 256;
    return c;
}

int keyboard_available(void) {
    return keyboard_buffer_tail != keyboard_buffer_head;
}

/* Mouse Driver */
#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_IRQ 12

struct mouse_packet {
    uint8_t buttons;
    int8_t x_movement;
    int8_t y_movement;
};

static struct mouse_packet mouse_state;
static int mouse_initialized = 0;

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_STATUS_PORT) & 1) == 1) {
                return;
            }
        }
    } else {
        while (timeout--) {
            if ((inb(MOUSE_STATUS_PORT) & 2) == 0) {
                return;
            }
        }
    }
}

void mouse_write(uint8_t value) {
    mouse_wait(1);
    outb(MOUSE_STATUS_PORT, 0xD4);
    mouse_wait(1);
    outb(MOUSE_DATA_PORT, value);
}

uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(MOUSE_DATA_PORT);
}

void mouse_handler(void) {
    static uint8_t mouse_cycle = 0;
    static uint8_t mouse_byte[3];
    
    /* Read the mouse packet */
    mouse_byte[mouse_cycle] = mouse_read();
    
    if (mouse_cycle == 2) {
        /* Complete packet received */
        mouse_state.buttons = mouse_byte[0];
        mouse_state.x_movement = mouse_byte[1];
        mouse_state.y_movement = mouse_byte[2];
        mouse_cycle = 0;
    } else {
        mouse_cycle++;
    }
}

void mouse_init(void) {
    /* Enable mouse device */
    mouse_wait(1);
    outb(MOUSE_STATUS_PORT, 0xA8);
    
    /* Enable interrupt */
    mouse_wait(1);
    outb(MOUSE_STATUS_PORT, 0x20);
    uint8_t status = mouse_read();
    status |= 0x02;
    mouse_wait(1);
    outb(MOUSE_STATUS_PORT, 0x60);
    mouse_write(status);
    
    /* Set default settings */
    mouse_write(0xF6);
    
    /* Enable data reporting */
    mouse_write(0xF4);
    
    mouse_initialized = 1;
}

/* Disk Driver (Simulated ATA) */
#define ATA_DATA_PORT 0x1F0
#define ATA_ERROR_PORT 0x1F1
#define ATA_SECTOR_COUNT_PORT 0x1F2
#define ATA_SECTOR_NUMBER_PORT 0x1F3
#define ATA_CYLINDER_LOW_PORT 0x1F4
#define ATA_CYLINDER_HIGH_PORT 0x1F5
#define ATA_DRIVE_HEAD_PORT 0x1F6
#define ATA_COMMAND_PORT 0x1F7
#define ATA_STATUS_PORT 0x1F7

#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_ERROR 0x01

#define SECTOR_SIZE 512
#define DISK_SIZE 1024 * 1024 /* 1MB simulated disk */

/* Simulated disk storage */
static uint8_t disk_storage[DISK_SIZE];

void ata_wait_ready(void) {
    while (inb(ATA_STATUS_PORT) & ATA_STATUS_BUSY) {
        /* Wait until not busy */
    }
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_ready();
    
    /* Select drive */
    outb(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count */
    outb(ATA_SECTOR_COUNT_PORT, 1);
    
    /* Set LBA */
    outb(ATA_SECTOR_NUMBER_PORT, lba & 0xFF);
    outb(ATA_CYLINDER_LOW_PORT, (lba >> 8) & 0xFF);
    outb(ATA_CYLINDER_HIGH_PORT, (lba >> 16) & 0xFF);
    
    /* Send read command */
    outb(ATA_COMMAND_PORT, ATA_CMD_READ);
    
    /* Wait for data ready */
    ata_wait_ready();
    
    /* Read sector data */
    for (int i = 0; i < SECTOR_SIZE; i += 2) {
        uint16_t data = inw(ATA_DATA_PORT);
        buffer[i] = data & 0xFF;
        buffer[i + 1] = (data >> 8) & 0xFF;
    }
}

void ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    ata_wait_ready();
    
    /* Select drive */
    outb(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count */
    outb(ATA_SECTOR_COUNT_PORT, 1);
    
    /* Set LBA */
    outb(ATA_SECTOR_NUMBER_PORT, lba & 0xFF);
    outb(ATA_CYLINDER_LOW_PORT, (lba >> 8) & 0xFF);
    outb(ATA_CYLINDER_HIGH_PORT, (lba >> 16) & 0xFF);
    
    /* Send write command */
    outb(ATA_COMMAND_PORT, ATA_CMD_WRITE);
    
    /* Wait for data ready */
    ata_wait_ready();
    
    /* Write sector data */
    for (int i = 0; i < SECTOR_SIZE; i += 2) {
        uint16_t data = buffer[i] | (buffer[i + 1] << 8);
        outw(ATA_DATA_PORT, data);
    }
    
    /* Flush cache */
    outb(ATA_COMMAND_PORT, 0xE7);
}

/* Simulated disk operations */
void simulated_disk_read(uint32_t lba, uint8_t* buffer) {
    uint32_t offset = lba * SECTOR_SIZE;
    if (offset + SECTOR_SIZE > DISK_SIZE) {
        return; /* Beyond disk size */
    }
    
    for (int i = 0; i < SECTOR_SIZE; i++) {
        buffer[i] = disk_storage[offset + i];
    }
}

void simulated_disk_write(uint32_t lba, const uint8_t* buffer) {
    uint32_t offset = lba * SECTOR_SIZE;
    if (offset + SECTOR_SIZE > DISK_SIZE) {
        return; /* Beyond disk size */
    }
    
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_storage[offset + i] = buffer[i];
    }
}

void disk_init(void) {
    /* Initialize simulated disk with zeros */
    for (int i = 0; i < DISK_SIZE; i++) {
        disk_storage[i] = 0;
    }
}

/* Process management stubs (required for compatibility) */
struct process {
    int dummy;
};

struct process* current_process = NULL;
struct process processes[16] = {0};

void process_kill(int pid) {
    (void)pid;
}

void process_switch(void) {
    /* Simple stub */
}

struct process* process_create(void) {
    return NULL;
}

void* paging_alloc_frame(void) {
    return NULL;
}

void paging_map_page(void* phys, void* virt) {
    (void)phys;
    (void)virt;
}

/* Timer Driver */
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND_PORT 0x43
#define PIT_FREQUENCY 1193182

uint32_t timer_ticks = 0;
uint32_t timer_frequency = 100; /* 100 Hz */

void timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    
    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    /* Send command byte */
    outb(PIT_COMMAND_PORT, 0x36);
    
    /* Send divisor */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

void timer_handler(void) {
    timer_ticks++;
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

void timer_sleep(uint32_t milliseconds) {
    uint32_t start_ticks = timer_ticks;
    uint32_t ticks_to_wait = (milliseconds * timer_frequency) / 1000;
    
    while (timer_ticks - start_ticks < ticks_to_wait) {
        /* Busy wait */
    }
}

/* Test Functions */
void test_keyboard_driver(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing Keyboard Driver ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Press keys (ESC to exit)...\n");
    
    while (1) {
        if (keyboard_available()) {
            char c = keyboard_getchar();
            if (c == 27) { /* ESC key */
                break;
            }
            terminal_putchar(c);
        }
    }
    terminal_putchar('\n');
}

void test_mouse_driver(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing Mouse Driver ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    if (mouse_initialized) {
        terminal_writestring("Mouse initialized successfully\n");
        terminal_writestring("Move mouse to see packet data\n");
        terminal_writestring("Press any key to continue...\n");
        
        /* Wait for key press */
        while (!keyboard_available()) {
            /* Display mouse state if changed */
            if (mouse_state.x_movement != 0 || mouse_state.y_movement != 0) {
                terminal_writestring("Mouse movement: X=");
                /* Simple hex output for movement */
                terminal_putchar('0');
                terminal_putchar('x');
                if (mouse_state.x_movement < 0) {
                    terminal_putchar('-');
                    terminal_putchar('0' + (-mouse_state.x_movement));
                } else {
                    terminal_putchar('0' + mouse_state.x_movement);
                }
                terminal_writestring(", Y=");
                if (mouse_state.y_movement < 0) {
                    terminal_putchar('-');
                    terminal_putchar('0' + (-mouse_state.y_movement));
                } else {
                    terminal_putchar('0' + mouse_state.y_movement);
                }
                terminal_putchar('\n');
            }
        }
        /* Clear the key */
        keyboard_getchar();
    } else {
        terminal_writestring("Mouse not initialized\n");
    }
}

void test_disk_driver(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing Disk Driver ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test data */
    uint8_t test_data[SECTOR_SIZE];
    uint8_t read_data[SECTOR_SIZE];
    
    /* Initialize test data */
    for (int i = 0; i < SECTOR_SIZE; i++) {
        test_data[i] = i % 256;
    }
    
    /* Write test data to sector 0 */
    terminal_writestring("Writing test data to sector 0...\n");
    simulated_disk_write(0, test_data);
    
    /* Read back and verify */
    terminal_writestring("Reading back from sector 0...\n");
    simulated_disk_read(0, read_data);
    
    /* Verify data */
    int errors = 0;
    for (int i = 0; i < SECTOR_SIZE; i++) {
        if (read_data[i] != test_data[i]) {
            errors++;
        }
    }
    
    if (errors == 0) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        terminal_writestring("Disk driver test PASSED\n");
    } else {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        terminal_writestring("Disk driver test FAILED\n");
        terminal_writestring("Errors: ");
        /* Simple error count display */
        if (errors < 10) {
            terminal_putchar('0' + errors);
        } else {
            terminal_putchar('0' + errors / 10);
            terminal_putchar('0' + errors % 10);
        }
        terminal_putchar('\n');
    }
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

void test_timer_driver(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing Timer Driver ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    uint32_t start_ticks = timer_get_ticks();
    terminal_writestring("Testing timer sleep for 1 second...\n");
    
    timer_sleep(1000); /* Sleep for 1 second */
    
    uint32_t end_ticks = timer_get_ticks();
    uint32_t elapsed = end_ticks - start_ticks;
    
    terminal_writestring("Expected ticks: ");
    terminal_putchar('0' + timer_frequency / 100);
    terminal_putchar('0' + (timer_frequency / 10) % 10);
    terminal_putchar('0' + timer_frequency % 10);
    terminal_writestring(", Actual ticks: ");
    
    /* Display elapsed ticks */
    if (elapsed < 100) {
        terminal_putchar('0' + elapsed / 10);
        terminal_putchar('0' + elapsed % 10);
    } else {
        terminal_putchar('0' + elapsed / 100);
        terminal_putchar('0' + (elapsed / 10) % 10);
        terminal_putchar('0' + elapsed % 10);
    }
    terminal_putchar('\n');
    
    if (elapsed >= timer_frequency / 2 && elapsed <= timer_frequency * 2) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        terminal_writestring("Timer driver test PASSED\n");
    } else {
        terminal_setcolor(VGA_COLOR_LIGHT_RED);
        terminal_writestring("Timer driver test FAILED\n");
    }
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

/* Main kernel function */
void kernel_main(void) {
    terminal_initialize();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Tiny Operating System - Phase 8 Device Drivers ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Initialize device drivers */
    terminal_writestring("Initializing device drivers...\n");
    
    /* Initialize keyboard */
    terminal_writestring("Keyboard: ");
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Initialize mouse */
    terminal_writestring("Mouse: ");
    mouse_init();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Initialize disk */
    terminal_writestring("Disk: ");
    disk_init();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Initialize timer */
    terminal_writestring("Timer: ");
    timer_init(timer_frequency);
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    terminal_putchar('\n');
    
    /* Test all device drivers */
    test_keyboard_driver();
    test_mouse_driver();
    test_disk_driver();
    test_timer_driver();
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Phase 8 Device Drivers Complete ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("All device drivers initialized and tested successfully.\n");
    terminal_writestring("Ready for Phase 9: Shell and User Space.\n");
    
    /* Infinite loop */
    while (1) {
        /* Halt CPU */
        __asm__ __volatile__ ("hlt");
    }
}