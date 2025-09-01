/*
 * Tiny Operating System - User Space System Call Handlers
 * C functions for handling system calls with user space support
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
    SYSCALL_BRK = 15,
    SYSCALL_MAX = 16
};

/* Memory management constants */
#define PAGE_SIZE 4096
#define PAGE_PRESENT    0x001
#define PAGE_WRITE      0x002
#define PAGE_USER       0x004

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
    uint32_t page_directory;
    uint32_t brk;
};

/* Terminal functions */
static void terminal_putchar(char c);
static void terminal_writestring(const char* data);
static void terminal_writehex(uint32_t value);

/* External variables */
extern uint32_t timer_ticks;
extern uint32_t timer_frequency;
extern struct process processes[16];
extern uint32_t current_process;

/* External functions */
uint32_t paging_alloc_frame(void);
void paging_free_frame(uint32_t addr);
void paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
uint32_t process_create(const char* name, uint32_t entry_point);
void process_switch(uint32_t pid);
void process_kill(uint32_t pid);

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

/* User space memory access validation */
static int validate_user_pointer(const void* ptr, uint32_t size) {
    uint32_t addr = (uint32_t)ptr;
    
    /* Check if pointer is in user space */
    if (addr >= 0xC0000000) {
        return 0;  /* Kernel space */
    }
    
    /* Check for overflow */
    if (addr + size < addr) {
        return 0;  /* Overflow */
    }
    
    /* Check if pointer is mapped */
    /* For now, assume it's valid if in user space */
    return 1;
}

/* Copy data from user space to kernel space */
static int copy_from_user(void* kernel_dest, const void* user_src, uint32_t size) {
    if (!validate_user_pointer(user_src, size)) {
        return -1;  /* Invalid user pointer */
    }
    
    /* Copy data byte by byte */
    uint8_t* dest = (uint8_t*)kernel_dest;
    const uint8_t* src = (const uint8_t*)user_src;
    
    for (uint32_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    
    return 0;
}


/* Get string length from user space */
static uint32_t strnlen_user(const char* user_str, uint32_t max_len) {
    uint32_t len = 0;
    
    while (len < max_len) {
        char c;
        if (copy_from_user(&c, user_str + len, 1) != 0) {
            break;
        }
        if (c == '\0') {
            break;
        }
        len++;
    }
    
    return len;
}

/* System call handler with user space support */
void syscall_handler_c(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    /* Suppress unused parameter warnings */
    (void)arg4;
    (void)arg5;
    
    switch (syscall_num) {
        case SYSCALL_EXIT: {
            /* Process exit */
            terminal_writestring("Process ");
            terminal_writehex(current_process);
            terminal_writestring(" exited with code ");
            terminal_writehex(arg1);
            terminal_writestring("\n");
            
            /* Kill the process */
            process_kill(current_process);
            
            /* Schedule another process */
            process_switch(0);  /* This will be handled by scheduler */
            break;
        }
            
        case SYSCALL_WRITE: {
            /* Write to file descriptor */
            if (arg1 == 1 || arg1 == 2) {  /* stdout or stderr */
                const char* user_buf = (const char*)arg2;
                uint32_t count = arg3;
                
                /* Validate user buffer */
                if (!validate_user_pointer(user_buf, count)) {
                    /* Return error */
                    __asm__ __volatile__("mov $0, %%eax" : : : "eax");
                    return;
                }
                
                /* Copy and write character by character */
                for (uint32_t i = 0; i < count; i++) {
                    char c;
                    if (copy_from_user(&c, user_buf + i, 1) == 0) {
                        terminal_putchar(c);
                    }
                }
                
                /* Return number of bytes written */
                __asm__ __volatile__("mov %0, %%eax" : : "r"(count) : "eax");
            } else {
                /* Invalid file descriptor */
                __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
            }
            break;
        }
            
        case SYSCALL_READ: {
            /* Read from file descriptor */
            if (arg1 == 0) {  /* stdin */
                /* For now, return 0 (no input available) */
                __asm__ __volatile__("mov $0, %%eax" : : : "eax");
            } else {
                /* Invalid file descriptor */
                __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
            }
            break;
        }
            
        case SYSCALL_GETPID: {
            /* Return current process PID */
            __asm__ __volatile__("mov %0, %%eax" : : "r"(processes[current_process].pid) : "eax");
            return;
        }
            
        case SYSCALL_SLEEP: {
            /* Sleep for specified milliseconds */
            if (timer_frequency > 0) {
                uint32_t sleep_ticks = arg1 * timer_frequency / 1000;  /* Convert ms to ticks */
                uint32_t start_ticks = timer_ticks;
                
                while (timer_ticks - start_ticks < sleep_ticks) {
                    __asm__ __volatile__("hlt");
                }
            }
            break;
        }
            
        case SYSCALL_FORK: {
            /* Create new process */
            uint32_t child_pid = process_create("child", processes[current_process].eip);
            if (child_pid) {
                __asm__ __volatile__("mov %0, %%eax" : : "r"(child_pid) : "eax");
            } else {
                __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
            }
            return;
        }
            
        case SYSCALL_EXEC: {
            /* Execute new program */
            const char* user_path = (const char*)arg1;
            
            /* Validate path string */
            if (!validate_user_pointer(user_path, 1)) {
                __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
                return;
            }
            
            /* Get path length */
            uint32_t path_len = strnlen_user(user_path, 256);
            if (path_len == 0) {
                __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
                return;
            }
            
            /* For now, just print the path */
            terminal_writestring("Exec: ");
            for (uint32_t i = 0; i < path_len && i < 256; i++) {
                char c;
                if (copy_from_user(&c, user_path + i, 1) == 0) {
                    terminal_putchar(c);
                }
            }
            terminal_writestring("\n");
            
            /* Return success */
            __asm__ __volatile__("mov $0, %%eax" : : : "eax");
            break;
        }
            
        case SYSCALL_BRK: {
            /* Change program break */
            uint32_t new_brk = arg1;
            
            if (new_brk == 0) {
                /* Return current break */
                __asm__ __volatile__("mov %0, %%eax" : : "r"(processes[current_process].brk) : "eax");
                return;
            }
            
            /* Validate new break */
            if (new_brk < processes[current_process].brk) {
                /* Can only decrease break for now */
                __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
                return;
            }
            
            /* Allocate pages as needed */
            uint32_t current_brk = processes[current_process].brk;
            while (current_brk < new_brk) {
                uint32_t page_frame = paging_alloc_frame();
                if (!page_frame) {
                    /* Out of memory */
                    __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
                    return;
                }
                
                /* Map page to user space */
                paging_map_page(current_brk, page_frame, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
                current_brk += PAGE_SIZE;
            }
            
            /* Update break */
            processes[current_process].brk = new_brk;
            
            /* Return success */
            __asm__ __volatile__("mov $0, %%eax" : : : "eax");
            break;
        }
            
        case SYSCALL_YIELD: {
            /* Yield CPU to another process */
            process_switch(0);  /* Let scheduler decide */
            break;
        }
            
        default:
            terminal_writestring("Unknown system call: ");
            terminal_writehex(syscall_num);
            terminal_writestring("\n");
            
            /* Return error */
            __asm__ __volatile__("mov $-1, %%eax" : : : "eax");
            break;
    }
}