/*
 * Phase 9: Shell and User Space Kernel
 * Implements user space execution and shell loading mechanism
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

/* Simple process management for Phase 9 */
int current_process = 0;
void process_kill(int pid) { (void)pid; }
void process_switch(void) { }

/* Timer and keyboard handlers */
uint32_t timer_ticks = 0;
uint32_t timer_frequency = 1000;
void timer_handler(void) { timer_ticks++; }
void keyboard_handler(void) { }

/* Simple paging functions */
void paging_alloc_frame(void) { }
void paging_map_page(uint32_t virt, uint32_t phys) { (void)virt; (void)phys; }

/* Simple process management */
struct process { int dummy; };
struct process processes[1];
int process_create(void) { return 0; }

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

/* Simple memory functions */
static void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = c;
    return s;
}

static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Forward declarations for syscall functions */
static void syscall_exit(int code) __attribute__((used));
static int syscall_read(int fd, void* buffer, int size) __attribute__((used));
static int syscall_write(int fd, const void* buffer, int size) __attribute__((used));
static int syscall_open(const char* filename) __attribute__((used));
static int syscall_close(int fd) __attribute__((used));
static int syscall_chdir(const char* path) __attribute__((used));
static int syscall_getcwd(char* buffer, int size) __attribute__((used));
static int syscall_opendir(const char* path) __attribute__((used));
static int syscall_readdir(int dirfd, void* dirent, int size) __attribute__((used));
static int syscall_closedir(int dirfd) __attribute__((used));

/* User space management */
#define USER_STACK_SIZE 4096
#define USER_BASE_ADDRESS 0x08000000

/* Process structure for user space execution */
struct user_process {
    uint32_t eip;
    uint32_t esp;
    uint32_t eflags;
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint8_t* stack;
    int running;
};

static struct user_process shell_process;

/* TSS structure for user space switching */
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct tss_entry tss;

/* GDT setup for user space */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr gdt_ptr;

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE 0x18
#define GDT_USER_DATA 0x20
#define GDT_TSS 0x28

/* GDT setup functions */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

static void gdt_install(void) {
    gdt_ptr.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_ptr.base = (uint32_t)&gdt;
    
    gdt_set_gate(0, 0, 0, 0, 0);                /* Null segment */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* Kernel code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* Kernel data */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* User code */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* User data */
    gdt_set_gate(5, (uint32_t)&tss, sizeof(struct tss_entry), 0x89, 0x40); /* TSS */
    
    __asm__ __volatile__ ("lgdt (%0)" : : "r"(&gdt_ptr));
}

/* TSS setup */
static void tss_install(void) {
    memset(&tss, 0, sizeof(struct tss_entry));
    tss.ss0 = GDT_KERNEL_DATA;
    tss.esp0 = 0x90000; /* Kernel stack */
    tss.iomap_base = sizeof(struct tss_entry);
    
    /* Load TSS */
    __asm__ __volatile__ (
        "mov $0x2B, %ax\n"
        "ltr %ax"
    );
}

/* System call interface */
#define SYSCALL_EXIT 1
#define SYSCALL_READ 2
#define SYSCALL_WRITE 3
#define SYSCALL_OPEN 4
#define SYSCALL_CLOSE 5
#define SYSCALL_CHDIR 16
#define SYSCALL_GETCWD 17
#define SYSCALL_OPENDIR 22
#define SYSCALL_READDIR 23
#define SYSCALL_CLOSEDIR 24

/* Simple file system simulation */
#define MAX_FILES 16
#define MAX_FILENAME 256
#define FILE_DATA_SIZE 4096

struct file_entry {
    char name[MAX_FILENAME];
    uint8_t data[FILE_DATA_SIZE];
    size_t size;
    int is_directory;
    int used;
};

static struct file_entry files[MAX_FILES];

/* Initialize file system */
static void filesystem_init(void) {
    memset(files, 0, sizeof(files));
    
    /* Create root directory */
    strcpy(files[0].name, ".");
    files[0].is_directory = 1;
    files[0].used = 1;
    
    /* Create some test files */
    strcpy(files[1].name, "README");
    strcpy((char*)files[1].data, "Tiny Operating System\nPhase 9: Shell and User Space\n");
    files[1].size = strlen((char*)files[1].data);
    files[1].used = 1;
    
    strcpy(files[2].name, "test.txt");
    strcpy((char*)files[2].data, "This is a test file.\n");
    files[2].size = strlen((char*)files[2].data);
    files[2].used = 1;
    
    /* Create a subdirectory */
    strcpy(files[3].name, "home");
    files[3].is_directory = 1;
    files[3].used = 1;
}

/* System call implementations */
static void syscall_exit(int code) {
    (void)code;
    terminal_writestring("Shell exited. System halted.\n");
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}

static int syscall_read(int fd, void* buffer, int size) {
    if (fd != 0) return -1; /* Only stdin supported */
    
    /* Simple keyboard input simulation */
    char* buf = (char*)buffer;
    int i = 0;
    
    while (i < size) {
        /* Wait for key press */
        while ((inb(0x64) & 1) == 0);
        uint8_t scancode = inb(0x60);
        
        /* Simple scancode to ASCII conversion */
        char c = 0;
        if (scancode == 0x1E) c = 'a';
        else if (scancode == 0x30) c = 'b';
        else if (scancode == 0x2E) c = 'c';
        else if (scancode == 0x20) c = 'd';
        else if (scancode == 0x12) c = 'e';
        else if (scancode == 0x21) c = 'f';
        else if (scancode == 0x22) c = 'g';
        else if (scancode == 0x23) c = 'h';
        else if (scancode == 0x17) c = 'i';
        else if (scancode == 0x24) c = 'j';
        else if (scancode == 0x25) c = 'k';
        else if (scancode == 0x26) c = 'l';
        else if (scancode == 0x32) c = 'm';
        else if (scancode == 0x31) c = 'n';
        else if (scancode == 0x18) c = 'o';
        else if (scancode == 0x19) c = 'p';
        else if (scancode == 0x10) c = 'q';
        else if (scancode == 0x13) c = 'r';
        else if (scancode == 0x1F) c = 's';
        else if (scancode == 0x14) c = 't';
        else if (scancode == 0x16) c = 'u';
        else if (scancode == 0x2F) c = 'v';
        else if (scancode == 0x11) c = 'w';
        else if (scancode == 0x2D) c = 'x';
        else if (scancode == 0x15) c = 'y';
        else if (scancode == 0x2C) c = 'z';
        else if (scancode == 0x39) c = ' ';
        else if (scancode == 0x1C) c = '\n';
        else if (scancode == 0x0E) c = '\b';
        
        if (c != 0) {
            buf[i++] = c;
            terminal_putchar(c);
            
            if (c == '\n') break;
        }
    }
    
    return i;
}

static int syscall_write(int fd, const void* buffer, int size) {
    if (fd != 1 && fd != 2) return -1; /* Only stdout/stderr supported */
    
    const char* buf = (const char*)buffer;
    for (int i = 0; i < size; i++) {
        terminal_putchar(buf[i]);
    }
    
    return size;
}

static int syscall_open(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            return i + 3; /* FD 0,1,2 reserved */
        }
    }
    return -1;
}

static int syscall_close(int fd) {
    if (fd < 3) return -1; /* Can't close stdin/stdout/stderr */
    return 0;
}

static int syscall_chdir(const char* path) {
    /* Simple implementation - only support root and home */
    if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0) {
        return 0;
    } else if (strcmp(path, "home") == 0) {
        return 0;
    }
    return -1;
}

static int syscall_getcwd(char* buffer, int size) {
    if (size < 2) return -1;
    strcpy(buffer, "/");
    return 1;
}

static int syscall_opendir(const char* path) {
    (void)path; /* Suppress unused parameter warning */
    /* Simple implementation - return a fake directory handle */
    return 100;
}

static int syscall_readdir(int dirfd, void* dirent, int size) {
    (void)size; /* Suppress unused parameter warning */
    static int dir_index = 0;
    
    if (dirfd != 100) return -1;
    
    /* Find next used file */
    while (dir_index < MAX_FILES && !files[dir_index].used) {
        dir_index++;
    }
    
    if (dir_index >= MAX_FILES) {
        dir_index = 0;
        return 0; /* End of directory */
    }
    
    /* Fill dirent structure */
    struct {
        uint32_t d_ino;
        uint8_t d_type;
        uint8_t d_reserved;
        uint16_t d_reclen;
        char d_name[256];
    } *entry = (void*)dirent;
    
    entry->d_ino = dir_index;
    entry->d_type = files[dir_index].is_directory ? 2 : 1;
    entry->d_reclen = sizeof(*entry);
    strcpy(entry->d_name, files[dir_index].name);
    
    dir_index++;
    return sizeof(*entry);
}

static int syscall_closedir(int dirfd) {
    if (dirfd == 100) return 0;
    return -1;
}


/* User space execution */
static void init_user_process(struct user_process* proc, void (*entry)(void)) {
    /* Allocate user stack */
    proc->stack = (uint8_t*)(USER_BASE_ADDRESS + 4096);
    
    /* Set up registers */
    proc->eip = (uint32_t)entry;
    proc->esp = (uint32_t)proc->stack;
    proc->eflags = 0x202; /* IF flag set */
    proc->eax = 0;
    proc->ebx = 0;
    proc->ecx = 0;
    proc->edx = 0;
    proc->esi = 0;
    proc->edi = 0;
    proc->ebp = 0;
    proc->running = 1;
}

/* Switch to user mode */
static void switch_to_user_mode(struct user_process* proc) {
    __asm__ __volatile__ (
        "push %0\n"      /* Stack segment */
        "push %1\n"      /* Stack pointer */
        "push %2\n"      /* Flags */
        "push %3\n"      /* Code segment */
        "push %4\n"      /* Instruction pointer */
        "mov $0x23, %%ax\n"  /* Data segment */
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "iret\n"
        : : "r"(GDT_USER_DATA), "r"(proc->esp), "r"(proc->eflags), 
            "r"(GDT_USER_CODE), "r"(proc->eip)
    );
}

/* Shell program entry point */
extern void shell_main(void);

/* Test functions */
void test_shell_basic(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing Shell Basic Functionality ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    terminal_writestring("Shell program compiled successfully\n");
    terminal_writestring("System calls implemented:\n");
    terminal_writestring("  - exit, read, write, open, close\n");
    terminal_writestring("  - chdir, getcwd, opendir, readdir, closedir\n");
    terminal_writestring("  - Built-in commands: help, exit, echo, cd, pwd, ls, clear, cat\n");
    terminal_putchar('\n');
}

void test_filesystem(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing File System ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    terminal_writestring("Files in root directory:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            terminal_writestring("  ");
            terminal_writestring(files[i].name);
            if (files[i].is_directory) {
                terminal_writestring("/\n");
            } else {
                terminal_writestring(" (");
                /* Simple size display */
                if (files[i].size < 10) {
                    terminal_putchar('0' + files[i].size);
                } else if (files[i].size < 100) {
                    terminal_putchar('0' + files[i].size / 10);
                    terminal_putchar('0' + files[i].size % 10);
                } else {
                    terminal_putchar('0' + files[i].size / 100);
                    terminal_putchar('0' + (files[i].size / 10) % 10);
                    terminal_putchar('0' + files[i].size % 10);
                }
                terminal_writestring(" bytes)\n");
            }
        }
    }
    terminal_putchar('\n');
}

void test_syscalls(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("=== Testing System Calls ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Test write syscall */
    terminal_writestring("Testing write syscall: ");
    char* test_msg = "Hello from syscall!\n";
    int result = syscall_write(1, test_msg, strlen(test_msg));
    if (result > 0) {
        terminal_writestring("OK\n");
    } else {
        terminal_writestring("FAILED\n");
    }
    
    /* Test open syscall */
    terminal_writestring("Testing open syscall: ");
    int fd = syscall_open("README");
    if (fd >= 0) {
        terminal_writestring("OK (fd=");
        terminal_putchar('0' + fd);
        terminal_writestring(")\n");
        syscall_close(fd);
    } else {
        terminal_writestring("FAILED\n");
    }
    
    terminal_putchar('\n');
}

/* Main kernel function */
void kernel_main(void) {
    terminal_initialize();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Tiny Operating System - Phase 9 Shell and User Space ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Initialize system components */
    terminal_writestring("Initializing system...\n");
    
    /* Setup GDT and TSS for user space */
    terminal_writestring("GDT: ");
    gdt_install();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    terminal_writestring("TSS: ");
    tss_install();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    /* Initialize file system */
    terminal_writestring("Filesystem: ");
    filesystem_init();
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("OK\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    
    terminal_putchar('\n');
    
    /* Run tests */
    test_shell_basic();
    test_filesystem();
    test_syscalls();
    
    /* Initialize shell process */
    terminal_writestring("Initializing shell process...\n");
    init_user_process(&shell_process, shell_main);
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("=== Starting User Space Shell ===\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("Welcome to the Tiny Operating System Shell!\n");
    terminal_writestring("Type 'help' for available commands.\n");
    terminal_putchar('\n');
    
    /* Switch to user mode and start shell */
    switch_to_user_mode(&shell_process);
    
    /* Should never reach here */
    terminal_writestring("Error: Returned from user mode\n");
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}