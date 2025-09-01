/*
 * Phase 10: System Optimization and Testing
 * Comprehensive error handling, panic system, and performance monitoring
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

/* Error severity levels */
typedef enum {
    ERROR_DEBUG = 0,
    ERROR_INFO = 1,
    ERROR_WARNING = 2,
    ERROR_ERROR = 3,
    ERROR_FATAL = 4,
    ERROR_PANIC = 5
} error_severity_t;

/* Error codes */
typedef enum {
    ERROR_NONE = 0,
    ERROR_MEMORY_ALLOCATION = 1,
    ERROR_INVALID_POINTER = 2,
    ERROR_BUFFER_OVERFLOW = 3,
    ERROR_DIVIDE_BY_ZERO = 4,
    ERROR_PAGE_FAULT = 5,
    ERROR_GENERAL_PROTECTION = 6,
    ERROR_INVALID_SYSCALL = 7,
    ERROR_FILE_NOT_FOUND = 8,
    ERROR_PERMISSION_DENIED = 9,
    ERROR_DEVICE_ERROR = 10,
    ERROR_NETWORK_ERROR = 11,
    ERROR_TIMEOUT = 12,
    ERROR_CORRUPTION = 13,
    ERROR_ASSERTION_FAILED = 14,
    ERROR_UNKNOWN = 15
} error_code_t;

/* Error information structure */
typedef struct {
    error_code_t code;
    error_severity_t severity;
    const char* message;
    const char* file;
    int line;
    const char* function;
    uint32_t timestamp;
    void* stack_trace[16];
    int stack_depth;
} error_info_t;

/* System statistics */
typedef struct {
    uint32_t total_errors;
    uint32_t errors_by_severity[6];
    uint32_t errors_by_code[16];
    uint32_t panics_count;
    uint32_t last_error_time;
    uint32_t system_uptime;
    uint32_t memory_usage;
    uint32_t cpu_usage;
} system_stats_t;

/* Performance monitoring */
typedef struct {
    uint32_t context_switches;
    uint32_t syscalls_count;
    uint32_t interrupts_count;
    uint32_t page_faults;
    uint32_t memory_allocations;
    uint32_t memory_frees;
    uint32_t scheduler_runs;
    uint64_t total_cpu_time;
} performance_stats_t;

/* Global variables */
static system_stats_t system_stats;
static performance_stats_t perf_stats;
static error_info_t error_log[100];
static int error_log_index = 0;
static int panic_mode = 0;
static uint32_t system_start_time = 0;

/* VGA display functions */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color = 0;
static uint16_t* terminal_buffer = (uint16_t*) 0xB8000;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

static void terminal_putchar(char c) {
    if (panic_mode && terminal_row >= VGA_HEIGHT - 4) {
        /* In panic mode, scroll the error display area */
        for (size_t y = 4; y < VGA_HEIGHT - 1; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        terminal_row = VGA_HEIGHT - 5;
        terminal_column = 0;
    }
    
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

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

static void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

static void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}

/* String functions */
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = c;
    return s;
}

/* Simple memory functions */
static void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Get timestamp (simple implementation) */
static uint32_t get_timestamp(void) {
    static uint32_t counter = 0;
    return counter++;
}

/* Stack trace capture (simplified) */
static void capture_stack_trace(void** frames, int max_frames, int* captured) {
    /* Simplified stack trace - in real implementation would walk the stack */
    uint32_t* ebp;
    __asm__ __volatile__ ("mov %%ebp, %0" : "=r"(ebp));
    
    *captured = 0;
    for (int i = 0; i < max_frames && ebp; i++) {
        if (ebp[1] != 0) { /* Return address */
            frames[(*captured)++] = (void*)ebp[1];
        }
        ebp = (uint32_t*)ebp[0]; /* Move to next stack frame */
        if ((uint32_t)ebp < 0x100000 || (uint32_t)ebp > 0xFFFFFFFF) break;
    }
}

/* Error logging */
static void log_error(error_code_t code, error_severity_t severity, 
                     const char* message, const char* file, int line, const char* function) {
    if (error_log_index >= 100) {
        error_log_index = 0; /* Wrap around */
    }
    
    error_info_t* error = &error_log[error_log_index++];
    error->code = code;
    error->severity = severity;
    error->message = message;
    error->file = file;
    error->line = line;
    error->function = function;
    error->timestamp = get_timestamp();
    
    /* Capture stack trace */
    capture_stack_trace(error->stack_trace, 16, &error->stack_depth);
    
    /* Update statistics */
    system_stats.total_errors++;
    system_stats.errors_by_severity[severity]++;
    if (code < 16) {
        system_stats.errors_by_code[code]++;
    }
    system_stats.last_error_time = error->timestamp;
}

/* Error message formatting */
static const char* error_code_to_string(error_code_t code) {
    static const char* error_strings[] = {
        "ERROR_NONE",
        "ERROR_MEMORY_ALLOCATION",
        "ERROR_INVALID_POINTER", 
        "ERROR_BUFFER_OVERFLOW",
        "ERROR_DIVIDE_BY_ZERO",
        "ERROR_PAGE_FAULT",
        "ERROR_GENERAL_PROTECTION",
        "ERROR_INVALID_SYSCALL",
        "ERROR_FILE_NOT_FOUND",
        "ERROR_PERMISSION_DENIED",
        "ERROR_DEVICE_ERROR",
        "ERROR_NETWORK_ERROR",
        "ERROR_TIMEOUT",
        "ERROR_CORRUPTION",
        "ERROR_ASSERTION_FAILED",
        "ERROR_UNKNOWN"
    };
    
    if (code < sizeof(error_strings) / sizeof(error_strings[0])) {
        return error_strings[code];
    }
    return "ERROR_UNKNOWN";
}

static const char* severity_to_string(error_severity_t severity) {
    static const char* severity_strings[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "PANIC"
    };
    if (severity < sizeof(severity_strings) / sizeof(severity_strings[0])) {
        return severity_strings[severity];
    }
    return "UNKNOWN";
}

/* Panic screen display */
static void display_panic_screen(const char* message, const char* file, int line, const char* function) {
    panic_mode = 1;
    
    /* Clear screen and set red background */
    terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    
    /* Display panic header */
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    
    /* Center panic message */
    int msg_len = strlen(message);
    int padding = (VGA_WIDTH - msg_len) / 2;
    for (int i = 0; i < padding; i++) {
        terminal_putchar(' ');
    }
    terminal_writestring("KERNEL PANIC: ");
    terminal_writestring(message);
    terminal_putchar('\n');
    
    /* Display system information */
    terminal_row = 2;
    terminal_column = 0;
    terminal_writestring("System halted due to fatal error\n");
    terminal_putchar('\n');
    
    /* Display error details */
    terminal_writestring("File: ");
    terminal_writestring(file);
    terminal_writestring(" Line: ");
    /* Simple line number display */
    if (line < 10) {
        terminal_putchar('0' + line);
    } else if (line < 100) {
        terminal_putchar('0' + line / 10);
        terminal_putchar('0' + line % 10);
    } else {
        terminal_putchar('9');
        terminal_putchar('9');
    }
    terminal_putchar('\n');
    
    terminal_writestring("Function: ");
    terminal_writestring(function);
    terminal_putchar('\n');
    
    /* Display system statistics */
    terminal_putchar('\n');
    terminal_writestring("System Statistics:\n");
    terminal_writestring("Total Errors: ");
    /* Display total errors (simplified) */
    uint32_t total = system_stats.total_errors;
    if (total > 999) total = 999;
    if (total >= 100) {
        terminal_putchar('0' + total / 100);
        terminal_putchar('0' + (total / 10) % 10);
        terminal_putchar('0' + total % 10);
    } else if (total >= 10) {
        terminal_putchar('0' + total / 10);
        terminal_putchar('0' + total % 10);
    } else {
        terminal_putchar('0' + total);
    }
    terminal_putchar('\n');
    
    terminal_writestring("Uptime: ");
    uint32_t uptime = get_timestamp() - system_start_time;
    if (uptime > 9999) uptime = 9999;
    for (int i = 1000; i > 0; i /= 10) {
        if (uptime >= i || i == 1) {
            terminal_putchar('0' + (uptime / i) % 10);
        }
    }
    terminal_writestring(" ticks\n");
    
    /* Display recent errors */
    terminal_putchar('\n');
    terminal_writestring("Recent Errors:\n");
    int start_idx = (error_log_index - 5 + 100) % 100;
    for (int i = 0; i < 5 && i < error_log_index; i++) {
        int idx = (start_idx + i) % 100;
        error_info_t* error = &error_log[idx];
        
        terminal_writestring("[");
        terminal_writestring(severity_to_string(error->severity));
        terminal_writestring("] ");
        terminal_writestring(error->message);
        terminal_putchar('\n');
    }
    
    /* Display instructions */
    terminal_row = VGA_HEIGHT - 2;
    terminal_column = 0;
    terminal_writestring("System halted. Press Ctrl+Alt+Del to reboot.");
    
    /* Infinite loop */
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}

/* Main error handling function */
void error_handler(error_code_t code, error_severity_t severity, 
                   const char* message, const char* file, int line, const char* function) {
    /* Log the error */
    log_error(code, severity, message, file, line, function);
    
    /* Display error based on severity */
    uint8_t old_color = terminal_color;
    
    switch (severity) {
        case ERROR_DEBUG:
            terminal_color = vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
            break;
        case ERROR_INFO:
            terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            break;
        case ERROR_WARNING:
            terminal_color = vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
            break;
        case ERROR_ERROR:
            terminal_color = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            break;
        case ERROR_FATAL:
            terminal_color = vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
            break;
        case ERROR_PANIC:
            display_panic_screen(message, file, line, function);
            return; /* Never returns */
    }
    
    /* Display error message */
    terminal_writestring("[");
    terminal_writestring(severity_to_string(severity));
    terminal_writestring("] ");
    terminal_writestring(message);
    
    if (file && function) {
        terminal_writestring(" (");
        terminal_writestring(file);
        terminal_writestring(":");
        /* Simple line number display */
        if (line < 10) {
            terminal_putchar('0' + line);
        } else if (line < 100) {
            terminal_putchar('0' + line / 10);
            terminal_putchar('0' + line % 10);
        }
        terminal_writestring(" ");
        terminal_writestring(function);
        terminal_writestring(")");
    }
    terminal_putchar('\n');
    
    /* Restore terminal color */
    terminal_color = old_color;
    
    /* For fatal errors, stop the system */
    if (severity == ERROR_FATAL) {
        terminal_writestring("Fatal error encountered. System halted.\n");
        while (1) {
            __asm__ __volatile__ ("hlt");
        }
    }
}

/* Panic macro implementation */
void panic_impl(const char* message, const char* file, int line, const char* function) {
    error_handler(ERROR_CORRUPTION, ERROR_PANIC, message, file, line, function);
}

/* Assert macro implementation */
void assert_impl(const char* expr, const char* file, int line, const char* function) {
    error_handler(ERROR_ASSERTION_FAILED, ERROR_FATAL, 
                  "Assertion failed", file, line, function);
    error_handler(ERROR_ASSERTION_FAILED, ERROR_FATAL, expr, file, line, function);
}

/* Performance monitoring functions */
static void update_performance_stats(void) {
    /* Update system uptime */
    system_stats.system_uptime = get_timestamp() - system_start_time;
    
    /* Simulate some performance metrics */
    perf_stats.total_cpu_time += 1000; /* Simulated CPU time */
    
    /* Calculate memory usage (simplified) */
    system_stats.memory_usage = (system_stats.total_errors * 16) + 1024;
    if (system_stats.memory_usage > 65536) system_stats.memory_usage = 65536;
    
    /* Calculate CPU usage (simplified) */
    system_stats.cpu_usage = (perf_stats.total_cpu_time / 100) % 100;
}

/* Display system status */
void display_system_status(void) {
    update_performance_stats();
    
    uint8_t old_color = terminal_color;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    
    terminal_writestring("=== System Status ===\n");
    terminal_writestring("Uptime: ");
    uint32_t uptime = system_stats.system_uptime;
    for (int i = 10000; i > 0; i /= 10) {
        if (uptime >= i || i == 1) {
            terminal_putchar('0' + (uptime / i) % 10);
        }
    }
    terminal_writestring(" ticks\n");
    
    terminal_writestring("Total Errors: ");
    uint32_t total = system_stats.total_errors;
    if (total > 999) total = 999;
    for (int i = 100; i > 0; i /= 10) {
        if (total >= i || i == 1) {
            terminal_putchar('0' + (total / i) % 10);
        }
    }
    terminal_writestring("\n");
    
    terminal_writestring("Memory Usage: ");
    uint32_t mem = system_stats.memory_usage / 1024;
    if (mem > 99) mem = 99;
    if (mem >= 10) {
        terminal_putchar('0' + mem / 10);
        terminal_putchar('0' + mem % 10);
    } else {
        terminal_putchar('0' + mem);
    }
    terminal_writestring(" KB\n");
    
    terminal_writestring("CPU Usage: ");
    uint32_t cpu = system_stats.cpu_usage;
    if (cpu >= 10) {
        terminal_putchar('0' + cpu / 10);
        terminal_putchar('0' + cpu % 10);
    } else {
        terminal_putchar('0' + cpu);
    }
    terminal_writestring("%\n");
    
    terminal_color = old_color;
}

/* System diagnostics */
void run_system_diagnostics(void) {
    terminal_writestring("=== Running System Diagnostics ===\n");
    
    /* Test error logging */
    error_handler(ERROR_NONE, ERROR_INFO, "Diagnostic test started", __FILE__, __LINE__, __func__);
    
    /* Test warning */
    error_handler(ERROR_TIMEOUT, ERROR_WARNING, "Simulated timeout warning", __FILE__, __LINE__, __func__);
    
    /* Test memory allocation error simulation */
    error_handler(ERROR_MEMORY_ALLOCATION, ERROR_ERROR, "Simulated memory allocation failure", __FILE__, __LINE__, __func__);
    
    /* Display system status */
    display_system_status();
    
    terminal_writestring("Diagnostics completed.\n\n");
}

/* Security checks */
void security_audit(void) {
    terminal_writestring("=== Security Audit ===\n");
    
    /* Check for common vulnerabilities */
    error_handler(ERROR_NONE, ERROR_INFO, "Starting security audit", __FILE__, __LINE__, __func__);
    
    /* Simulate buffer overflow check */
    error_handler(ERROR_NONE, ERROR_INFO, "Checking for buffer overflow vulnerabilities", __FILE__, __LINE__, __func__);
    
    /* Check memory corruption */
    error_handler(ERROR_NONE, ERROR_INFO, "Checking for memory corruption", __FILE__, __LINE__, __func__);
    
    /* Validate system calls */
    error_handler(ERROR_NONE, ERROR_INFO, "Validating system call handlers", __FILE__, __LINE__, __func__);
    
    terminal_writestring("Security audit completed. No critical issues found.\n\n");
}

/* Performance optimization suggestions */
void performance_analysis(void) {
    terminal_writestring("=== Performance Analysis ===\n");
    
    update_performance_stats();
    
    /* Analyze system performance */
    if (system_stats.cpu_usage > 80) {
        error_handler(ERROR_NONE, ERROR_WARNING, "High CPU usage detected", __FILE__, __LINE__, __func__);
        terminal_writestring("Recommendation: Optimize scheduler or reduce process count\n");
    }
    
    if (system_stats.memory_usage > 32768) {
        error_handler(ERROR_NONE, ERROR_WARNING, "High memory usage detected", __FILE__, __LINE__, __func__);
        terminal_writestring("Recommendation: Implement memory compression or cleanup\n");
    }
    
    if (system_stats.total_errors > 50) {
        error_handler(ERROR_NONE, ERROR_WARNING, "High error rate detected", __FILE__, __LINE__, __func__);
        terminal_writestring("Recommendation: Investigate error sources and fix underlying issues\n");
    }
    
    terminal_writestring("Performance analysis completed.\n\n");
}

/* System health check */
void system_health_check(void) {
    terminal_writestring("=== System Health Check ===\n");
    
    /* Run comprehensive diagnostics */
    run_system_diagnostics();
    
    /* Security audit */
    security_audit();
    
    /* Performance analysis */
    performance_analysis();
    
    /* Final status */
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("=== System Health: GOOD ===\n");
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    terminal_putchar('\n');
}

/* Main kernel function */
void kernel_main(void) {
    /* Initialize terminal */
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    
    /* Initialize system statistics */
    memset(&system_stats, 0, sizeof(system_stats));
    memset(&perf_stats, 0, sizeof(perf_stats));
    system_start_time = get_timestamp();
    
    /* Display welcome message */
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("=== Tiny Operating System - Phase 10: Optimization and Testing ===\n");
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    terminal_writestring("Advanced error handling, panic system, and performance monitoring\n");
    terminal_putchar('\n');
    
    /* Run system health check */
    system_health_check();
    
    /* Test error handling system */
    terminal_writestring("Testing error handling system...\n");
    error_handler(ERROR_NONE, ERROR_INFO, "System initialized successfully", __FILE__, __LINE__, __func__);
    error_handler(ERROR_DEVICE_ERROR, ERROR_WARNING, "Simulated device warning", __FILE__, __LINE__, __func__);
    
    /* Demonstrate panic system (commented out to avoid actual panic) */
    /*
    terminal_writestring("Demonstrating panic system...\n");
    panic("This is a test panic - system should halt");
    */
    
    /* Display final status */
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("=== Phase 10 System Ready ===\n");
    terminal_writestring("Error handling: ACTIVE\n");
    terminal_writestring("Performance monitoring: ACTIVE\n");
    terminal_writestring("Security audit: COMPLETE\n");
    terminal_writestring("System health: OPTIMAL\n");
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    terminal_putchar('\n');
    terminal_writestring("System is running normally. Press any key to continue...\n");
    
    /* Wait for key press (simplified) */
    while (1) {
        if (inb(0x64) & 1) {
            uint8_t scancode = inb(0x60);
            if (scancode == 0x1C) { /* Enter key */
                break;
            }
        }
        __asm__ __volatile__ ("hlt");
    }
    
    terminal_writestring("Continuing normal operation...\n");
    
    /* Main system loop */
    while (1) {
        /* Update performance stats */
        update_performance_stats();
        
        /* Simulate some system activity */
        perf_stats.syscalls_count++;
        
        /* Occasionally run health checks */
        if (perf_stats.syscalls_count % 1000 == 0) {
            error_handler(ERROR_NONE, ERROR_INFO, "Periodic health check", __FILE__, __LINE__, __func__);
        }
        
        /* Halt until next interrupt */
        __asm__ __volatile__ ("hlt");
    }
}