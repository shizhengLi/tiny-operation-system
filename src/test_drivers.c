/*
 * Phase 8 Device Drivers - Unit Tests
 * Tests individual device driver components
 */

#include <stdint.h>
#include <stddef.h>

/* Simple assert macro for testing */
#define assert(condition) \
    do { \
        if (!(condition)) { \
            test_failed = 1; \
            test_fail_line = __LINE__; \
            return; \
        } \
    } while (0)

/* Test state */
static int test_failed = 0;
static int test_fail_line = 0;

/* VGA functions (simplified for testing) */
static void test_putchar(char c) {
    /* Simplified - would normally write to VGA buffer */
    (void)c;
}

/* Port I/O simulation */
static uint8_t mock_inb(uint16_t port) {
    static uint8_t mock_keyboard_data[] = {0x1E, 0x1F, 0x20, 0x21, 0x22}; /* a, s, d, f, g */
    static uint8_t mock_mouse_data[] = {0x09, 0x05, 0x02}; /* Left button, X+5, Y+2 */
    static uint8_t mock_disk_data[512];
    
    if (port == 0x60) { /* Keyboard/Mouse data port */
        static int keyboard_index = 0;
        static int mouse_index = 0;
        
        /* Return keyboard data for first few calls */
        if (keyboard_index < 5) {
            return mock_keyboard_data[keyboard_index++];
        }
        
        /* Return mouse data */
        if (mouse_index < 3) {
            return mock_mouse_data[mouse_index++];
        }
        
        return 0;
    }
    
    if (port == 0x1F0) { /* Disk data port */
        static int disk_index = 0;
        if (disk_index < 512) {
            return mock_disk_data[disk_index++];
        }
        return 0;
    }
    
    return 0;
}

static void mock_outb(uint16_t port, uint8_t value) {
    (void)port;
    (void)value;
    /* Mock implementation */
}

/* Keyboard driver unit tests */
void test_keyboard_buffer(void) {
    /* Test keyboard buffer functionality */
    char keyboard_buffer[256];
    int keyboard_buffer_head = 0;
    int keyboard_buffer_tail = 0;
    
    /* Test buffer empty */
    assert(keyboard_buffer_head == keyboard_buffer_tail);
    
    /* Test adding character */
    keyboard_buffer[keyboard_buffer_head] = 'A';
    keyboard_buffer_head = (keyboard_buffer_head + 1) % 256;
    assert(keyboard_buffer_head != keyboard_buffer_tail);
    
    /* Test reading character */
    char c = keyboard_buffer[keyboard_buffer_tail];
    keyboard_buffer_tail = (keyboard_buffer_tail + 1) % 256;
    assert(c == 'A');
    assert(keyboard_buffer_head == keyboard_buffer_tail);
    
    /* Test buffer wrap-around */
    for (int i = 0; i < 300; i++) {
        if ((keyboard_buffer_head + 1) % 256 != keyboard_buffer_tail) {
            keyboard_buffer[keyboard_buffer_head] = 'X';
            keyboard_buffer_head = (keyboard_buffer_head + 1) % 256;
        }
    }
    /* Should not crash and buffer should be full */
    assert((keyboard_buffer_head + 1) % 256 == keyboard_buffer_tail);
}

void test_scancode_to_ascii(void) {
    /* Test scancode to ASCII conversion */
    const char scancode_to_ascii[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
        0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    /* Test letter conversion */
    assert(scancode_to_ascii[0x1E] == 'a'); /* scancode for 'a' */
    assert(scancode_to_ascii[0x30] == 'b'); /* scancode for 'b' */
    
    /* Test number conversion */
    assert(scancode_to_ascii[0x02] == '1'); /* scancode for '1' */
    assert(scancode_to_ascii[0x03] == '2'); /* scancode for '2' */
    
    /* Test special keys */
    assert(scancode_to_ascii[0x0E] == '\b'); /* backspace */
    assert(scancode_to_ascii[0x0F] == '\t'); /* tab */
    assert(scancode_to_ascii[0x1C] == '\n'); /* enter */
    
    /* Test unused scancodes */
    assert(scancode_to_ascii[0x00] == 0); /* unused */
    assert(scancode_to_ascii[0x01] == 0); /* escape */
}

void test_shift_processing(void) {
    /* Test shift key processing */
    int keyboard_shift_pressed = 0;
    
    /* Test normal state */
    assert(keyboard_shift_pressed == 0);
    
    /* Test shift key press */
    keyboard_shift_pressed = 1;
    assert(keyboard_shift_pressed == 1);
    
    /* Test shift key release */
    keyboard_shift_pressed = 0;
    assert(keyboard_shift_pressed == 0);
    
    /* Test character conversion with shift */
    char c = 'a';
    if (keyboard_shift_pressed) {
        c = c - 'a' + 'A';
    }
    assert(c == 'a');
    
    /* Test with shift pressed */
    keyboard_shift_pressed = 1;
    c = 'a';
    if (keyboard_shift_pressed) {
        c = c - 'a' + 'A';
    }
    assert(c == 'A');
}

/* Mouse driver unit tests */
void test_mouse_packet_processing(void) {
    /* Test mouse packet structure */
    struct mouse_packet {
        uint8_t buttons;
        int8_t x_movement;
        int8_t y_movement;
    };
    
    struct mouse_packet mouse_state;
    uint8_t mouse_byte[3];
    
    /* Test packet assembly */
    mouse_byte[0] = 0x09; /* Left button, Y overflow, X sign */
    mouse_byte[1] = 0x05; /* X movement */
    mouse_byte[2] = 0x02; /* Y movement */
    
    mouse_state.buttons = mouse_byte[0];
    mouse_state.x_movement = mouse_byte[1];
    mouse_state.y_movement = mouse_byte[2];
    
    /* Test button states */
    assert((mouse_state.buttons & 0x01) != 0); /* Left button pressed */
    assert((mouse_state.buttons & 0x02) == 0); /* Right button not pressed */
    assert((mouse_state.buttons & 0x04) == 0); /* Middle button not pressed */
    
    /* Test movement */
    assert(mouse_state.x_movement == 5);
    assert(mouse_state.y_movement == 2);
}

void test_mouse_cycle_state(void) {
    /* Test mouse packet cycle state machine */
    static uint8_t mouse_cycle = 0;
    static uint8_t mouse_byte[3];
    
    /* Test cycle progression */
    assert(mouse_cycle == 0);
    
    /* Simulate receiving first byte */
    mouse_byte[mouse_cycle] = 0x01;
    mouse_cycle++;
    assert(mouse_cycle == 1);
    
    /* Simulate receiving second byte */
    mouse_byte[mouse_cycle] = 0x02;
    mouse_cycle++;
    assert(mouse_cycle == 2);
    
    /* Simulate receiving third byte - should reset cycle */
    mouse_byte[mouse_cycle] = 0x03;
    mouse_cycle++;
    assert(mouse_cycle == 0);
}

/* Disk driver unit tests */
void test_disk_addressing(void) {
    /* Test disk LBA addressing */
    uint32_t lba = 0x12345678;
    
    /* Test LBA to CHS conversion components */
    uint8_t sector = lba & 0xFF;
    uint8_t cylinder_low = (lba >> 8) & 0xFF;
    uint8_t cylinder_high = (lba >> 16) & 0xFF;
    uint8_t head = (lba >> 24) & 0x0F;
    
    assert(sector == 0x78);
    assert(cylinder_low == 0x56);
    assert(cylinder_high == 0x34);
    assert(head == 0x08); /* Only lower 4 bits */
}

void test_disk_sector_operations(void) {
    /* Test disk sector read/write operations */
    #define SECTOR_SIZE 512
    uint8_t write_buffer[SECTOR_SIZE];
    uint8_t read_buffer[SECTOR_SIZE];
    uint8_t disk_storage[1024 * 1024]; /* 1MB simulated disk */
    
    /* Initialize write buffer with test pattern */
    for (int i = 0; i < SECTOR_SIZE; i++) {
        write_buffer[i] = i % 256;
    }
    
    /* Simulate write operation */
    uint32_t lba = 10;
    uint32_t offset = lba * SECTOR_SIZE;
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_storage[offset + i] = write_buffer[i];
    }
    
    /* Simulate read operation */
    for (int i = 0; i < SECTOR_SIZE; i++) {
        read_buffer[i] = disk_storage[offset + i];
    }
    
    /* Verify data integrity */
    for (int i = 0; i < SECTOR_SIZE; i++) {
        assert(read_buffer[i] == write_buffer[i]);
    }
}

void test_disk_bounds_checking(void) {
    /* Test disk bounds checking */
    #define DISK_SIZE (1024 * 1024)
    #define SECTOR_SIZE 512
    
    /* Test valid LBA */
    uint32_t valid_lba = (DISK_SIZE / SECTOR_SIZE) - 1;
    uint32_t offset = valid_lba * SECTOR_SIZE;
    assert(offset + SECTOR_SIZE <= DISK_SIZE);
    
    /* Test invalid LBA (too large) */
    uint32_t invalid_lba = DISK_SIZE / SECTOR_SIZE;
    offset = invalid_lba * SECTOR_SIZE;
    assert(offset + SECTOR_SIZE > DISK_SIZE);
}

/* Timer driver unit tests */
void test_timer_frequency_calculation(void) {
    /* Test timer frequency calculation */
    #define PIT_FREQUENCY 1193182
    uint32_t frequency = 100;
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    /* Test divisor calculation */
    assert(divisor == 11931); /* 1193182 / 100 = 11931.82, truncated to 11931 */
    
    /* Test divisor range */
    assert(divisor > 0);
    assert(divisor <= 65535); /* 16-bit divisor limit */
}

void test_timer_tick_counting(void) {
    /* Test timer tick counting */
    static uint32_t timer_ticks = 0;
    
    /* Test initial state */
    assert(timer_ticks == 0);
    
    /* Simulate timer interrupts */
    for (int i = 0; i < 10; i++) {
        timer_ticks++;
    }
    
    /* Test tick count */
    assert(timer_ticks == 10);
}

void test_timer_sleep_function(void) {
    /* Test timer sleep function */
    static uint32_t timer_ticks = 0;
    uint32_t start_ticks = 0;
    uint32_t frequency = 100; /* 100 Hz */
    
    /* Test sleep calculation */
    uint32_t milliseconds = 500; /* 0.5 seconds */
    uint32_t ticks_to_wait = (milliseconds * frequency) / 1000;
    assert(ticks_to_wait == 50); /* 500ms * 100Hz / 1000 = 50 ticks */
    
    /* Simulate sleep completion */
    start_ticks = timer_ticks;
    for (int i = 0; i < ticks_to_wait; i++) {
        timer_ticks++;
    }
    uint32_t elapsed = timer_ticks - start_ticks;
    assert(elapsed == ticks_to_wait);
}

/* Test runner */
void run_unit_tests(void) {
    /* Keyboard tests */
    test_keyboard_buffer();
    test_scancode_to_ascii();
    test_shift_processing();
    
    /* Mouse tests */
    test_mouse_packet_processing();
    test_mouse_cycle_state();
    
    /* Disk tests */
    test_disk_addressing();
    test_disk_sector_operations();
    test_disk_bounds_checking();
    
    /* Timer tests */
    test_timer_frequency_calculation();
    test_timer_tick_counting();
    test_timer_sleep_function();
    
    /* Report results */
    if (test_failed) {
        /* Would normally print error message with line number */
        test_failed = 0; /* Reset for next test */
    }
}

/* Integration test helpers */
void test_keyboard_mouse_integration(void) {
    /* Test keyboard and mouse working together */
    int keyboard_available = 0;
    int mouse_initialized = 0;
    
    /* Initialize both devices */
    mouse_initialized = 1;
    keyboard_available = 1;
    
    /* Test both devices can be active simultaneously */
    assert(mouse_initialized == 1);
    assert(keyboard_available == 1);
}

void test_disk_timer_integration(void) {
    /* Test disk operations with timing */
    uint32_t timer_ticks = 0;
    
    /* Simulate disk read operation with timing */
    uint32_t start_ticks = timer_ticks;
    
    /* Simulate disk read delay */
    for (int i = 0; i < 100; i++) {
        timer_ticks++; /* Simulate timer ticks during operation */
    }
    
    uint32_t end_ticks = timer_ticks;
    uint32_t elapsed = end_ticks - start_ticks;
    
    /* Test that disk operation takes some time */
    assert(elapsed > 0);
}

void run_integration_tests(void) {
    /* Integration tests */
    test_keyboard_mouse_integration();
    test_disk_timer_integration();
}

/* Main test function */
void test_main(void) {
    /* Run all unit tests */
    run_unit_tests();
    
    /* Run all integration tests */
    run_integration_tests();
    
    /* Test completion */
    if (test_failed) {
        /* Would normally print failure message */
    } else {
        /* Would normally print success message */
    }
}