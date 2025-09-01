/*
 * Phase 9: User Space Shell Program
 * Implements basic shell commands (ls, cd, echo) for user space execution
 */

#include <stdint.h>
#include <stddef.h>

/* System call numbers */
#define SYS_EXIT 1
#define SYS_READ 2
#define SYS_WRITE 3
#define SYS_OPEN 4
#define SYS_CLOSE 5
#define SYS_SEEK 6
#define SYS_MMAP 7
#define SYS_MUNMAP 8
#define SYS_FORK 9
#define SYS_EXEC 10
#define SYS_WAIT 11
#define SYS_KILL 12
#define SYS_GETPID 13
#define SYS_SLEEP 14
#define SYS_YIELD 15
#define SYS_CHDIR 16
#define SYS_GETCWD 17
#define SYS_MKDIR 18
#define SYS_RMDIR 19
#define SYS_UNLINK 20
#define SYS_RENAME 21
#define SYS_OPENDIR 22
#define SYS_READDIR 23
#define SYS_CLOSEDIR 24
#define SYS_STAT 25
#define SYS_FSTAT 26
#define SYS_ACCESS 27
#define SYS_PIPE 28
#define SYS_DUP 29
#define SYS_DUP2 30
#define SYS_FCNTL 31
#define SYS_IOCTL 32
#define SYS_SELECT 33
#define SYS_POLL 34
#define SYS_SOCKET 35
#define SYS_BIND 36
#define SYS_LISTEN 37
#define SYS_ACCEPT 38
#define SYS_CONNECT 39
#define SYS_SEND 40
#define SYS_RECV 41
#define SYS_SENDTO 42
#define SYS_RECVFROM 43
#define SYS_SHUTDOWN 44
#define SYS_GETSOCKOPT 45
#define SYS_SETSOCKOPT 46
#define SYS_GETPEERNAME 47
#define SYS_GETSOCKNAME 48
#define SYS_GETHOSTNAME 49
#define SYS_SETHOSTNAME 50

/* File descriptors */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* System call interface */
static inline int syscall0(int num) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

static inline int syscall1(int num, int arg1) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

static inline int syscall2(int num, int arg1, int arg2) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
    return ret;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/* Simple string functions */
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

/* Forward declarations */
static char* strtok_r(char* str, const char* delim, char** saveptr);

/* Simple memory functions */
static void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = c;
    return s;
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}


static char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

static char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n-- && (*d++ = *src++));
    while (n--) *d++ = '\0';
    return dest;
}


/* Directory entry structure */
struct dirent {
    uint32_t d_ino;        /* Inode number */
    uint8_t d_type;        /* File type */
    uint8_t d_reserved;    /* Reserved */
    uint16_t d_reclen;     /* Record length */
    char d_name[256];      /* Filename */
};

/* Shell command structure */
#define MAX_ARGS 32
#define MAX_CMD_LEN 256
#define MAX_PATH_LEN 256

struct command {
    char name[MAX_CMD_LEN];
    char* args[MAX_ARGS + 1]; /* +1 for NULL terminator */
    int argc;
    int background;
};

/* Shell state */
static char current_directory[MAX_PATH_LEN] = "/";
static char input_buffer[MAX_CMD_LEN];

/* Built-in commands */
static int builtin_help(int argc, char* argv[]);
static int builtin_exit(int argc, char* argv[]);
static int builtin_echo(int argc, char* argv[]);
static int builtin_cd(int argc, char* argv[]);
static int builtin_pwd(int argc, char* argv[]);
static int builtin_ls(int argc, char* argv[]);
static int builtin_clear(int argc, char* argv[]);
static int builtin_cat(int argc, char* argv[]);

/* Command table */
struct builtin_command {
    const char* name;
    int (*func)(int argc, char* argv[]);
    const char* help;
};

static struct builtin_command builtin_commands[] = {
    {"help", builtin_help, "Show available commands"},
    {"exit", builtin_exit, "Exit the shell"},
    {"echo", builtin_echo, "Echo arguments to output"},
    {"cd", builtin_cd, "Change directory"},
    {"pwd", builtin_pwd, "Print working directory"},
    {"ls", builtin_ls, "List directory contents"},
    {"clear", builtin_clear, "Clear screen"},
    {"cat", builtin_cat, "Display file contents"},
    {NULL, NULL, NULL}
};

/* Output functions */
static void shell_putchar(char c) {
    (void)c; /* Suppress unused parameter warning */
    syscall1(SYS_WRITE, STDOUT_FILENO);
    /* Note: This is simplified - real implementation would pass character */
    /* For now, we'll use the write system call with buffer */
}

static void shell_write(const char* str) {
    syscall2(SYS_WRITE, STDOUT_FILENO, (int)str);
}

static void shell_writeln(const char* str) {
    shell_write(str);
    shell_write("\n");
}


/* Input functions */
static char shell_getchar(void) {
    char c;
    if (syscall2(SYS_READ, STDIN_FILENO, (int)&c) > 0) {
        return c;
    }
    return 0;
}

static void shell_read_line(char* buffer, int max_len) {
    int pos = 0;
    
    while (pos < max_len - 1) {
        char c = shell_getchar();
        if (c == '\n' || c == '\r') {
            break;
        } else if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                shell_write("\b \b"); /* Backspace, space, backspace */
            }
        } else if (c >= 32 && c <= 126) {
            buffer[pos++] = c;
            shell_putchar(c);
        }
    }
    
    buffer[pos] = '\0';
    shell_write("\n");
}

/* Command parsing */
static void parse_command(const char* input, struct command* cmd) {
    char buffer[MAX_CMD_LEN];
    strncpy(buffer, input, MAX_CMD_LEN - 1);
    buffer[MAX_CMD_LEN - 1] = '\0';
    
    cmd->argc = 0;
    cmd->background = 0;
    
    char* token = buffer;
    char* saveptr;
    
    /* Parse command name */
    token = strtok_r(buffer, " \t\n", &saveptr);
    if (!token) return;
    
    strncpy(cmd->name, token, MAX_CMD_LEN - 1);
    cmd->name[MAX_CMD_LEN - 1] = '\0';
    cmd->args[cmd->argc++] = cmd->name;
    
    /* Parse arguments */
    while ((token = strtok_r(NULL, " \t\n", &saveptr)) && cmd->argc < MAX_ARGS) {
        cmd->args[cmd->argc++] = token;
    }
    
    /* Check for background execution */
    if (cmd->argc > 0 && strcmp(cmd->args[cmd->argc - 1], "&") == 0) {
        cmd->background = 1;
        cmd->argc--;
        cmd->args[cmd->argc] = NULL;
    } else {
        cmd->args[cmd->argc] = NULL;
    }
}

/* Built-in command implementations */
static int builtin_help(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    shell_writeln("Available built-in commands:");
    shell_writeln("");
    
    for (int i = 0; builtin_commands[i].name; i++) {
        shell_write("  ");
        shell_write(builtin_commands[i].name);
        for (int j = strlen(builtin_commands[i].name); j < 12; j++) {
            shell_write(" ");
        }
        shell_writeln(builtin_commands[i].help);
    }
    
    return 0;
}

static int builtin_exit(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    shell_writeln("Exiting shell...");
    syscall0(SYS_EXIT);
    return 0;
}

static int builtin_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) shell_write(" ");
        shell_write(argv[i]);
    }
    shell_write("\n");
    return 0;
}

static int builtin_cd(int argc, char* argv[]) {
    if (argc != 2) {
        shell_writeln("Usage: cd <directory>");
        return 1;
    }
    
    if (syscall1(SYS_CHDIR, (int)argv[1]) < 0) {
        shell_write("cd: ");
        shell_write(argv[1]);
        shell_writeln(": No such directory");
        return 1;
    }
    
    /* Update current directory */
    if (syscall2(SYS_GETCWD, (int)current_directory, MAX_PATH_LEN) < 0) {
        strcpy(current_directory, "/");
    }
    
    return 0;
}

static int builtin_pwd(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    if (syscall2(SYS_GETCWD, (int)current_directory, MAX_PATH_LEN) < 0) {
        strcpy(current_directory, "/");
    }
    
    shell_writeln(current_directory);
    return 0;
}

static int builtin_ls(int argc, char* argv[]) {
    const char* path = ".";
    
    if (argc > 1) {
        path = argv[1];
    }
    
    int dirfd = syscall1(SYS_OPENDIR, (int)path);
    if (dirfd < 0) {
        shell_write("ls: cannot access '");
        shell_write(path);
        shell_writeln("': No such directory");
        return 1;
    }
    
    struct dirent entry;
    memset(&entry, 0, sizeof(entry)); /* Initialize to avoid warning */
    while (syscall3(SYS_READDIR, dirfd, (int)&entry, sizeof(entry)) > 0) {
        if (entry.d_name[0] != '\0') {
            shell_write(entry.d_name);
            shell_write("\n");
        }
    }
    
    syscall1(SYS_CLOSEDIR, dirfd);
    return 0;
}

static int builtin_clear(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    shell_write("\033[2J\033[H"); /* ANSI clear screen */
    return 0;
}

static int builtin_cat(int argc, char* argv[]) {
    if (argc != 2) {
        shell_writeln("Usage: cat <file>");
        return 1;
    }
    
    int fd = syscall1(SYS_OPEN, (int)argv[1]);
    if (fd < 0) {
        shell_write("cat: ");
        shell_write(argv[1]);
        shell_writeln(": No such file");
        return 1;
    }
    
    char buffer[1024];
    int bytes_read;
    
    while ((bytes_read = syscall3(SYS_READ, fd, (int)buffer, sizeof(buffer))) > 0) {
        syscall2(SYS_WRITE, STDOUT_FILENO, (int)buffer);
    }
    
    syscall1(SYS_CLOSE, fd);
    return 0;
}

/* Simple strtok_r implementation */
static char* strtok_r(char* str, const char* delim, char** saveptr) {
    if (str == NULL) {
        str = *saveptr;
    }
    
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }
    
    char* token = str;
    while (*str) {
        const char* d = delim;
        while (*d) {
            if (*str == *d) {
                *str = '\0';
                *saveptr = str + 1;
                return token;
            }
            d++;
        }
        str++;
    }
    
    *saveptr = str;
    return token;
}

/* Command execution */
static int execute_command(struct command* cmd) {
    /* Check for built-in commands */
    for (int i = 0; builtin_commands[i].name; i++) {
        if (strcmp(cmd->name, builtin_commands[i].name) == 0) {
            return builtin_commands[i].func(cmd->argc, cmd->args);
        }
    }
    
    shell_write("shell: ");
    shell_write(cmd->name);
    shell_writeln(": command not found");
    return 127;
}

/* Shell prompt */
static void show_prompt(void) {
    if (syscall2(SYS_GETCWD, (int)current_directory, MAX_PATH_LEN) < 0) {
        strcpy(current_directory, "/");
    }
    
    shell_write("[");
    shell_write(current_directory);
    shell_write("]$ ");
}

/* Main shell loop */
static void shell_loop(void) {
    struct command cmd;
    
    while (1) {
        show_prompt();
        
        /* Read command */
        shell_read_line(input_buffer, MAX_CMD_LEN);
        
        /* Skip empty lines */
        if (strlen(input_buffer) == 0) {
            continue;
        }
        
        /* Parse command */
        parse_command(input_buffer, &cmd);
        
        /* Execute command */
        execute_command(&cmd);
    }
}

/* Shell entry point */
void shell_main(void) {
    shell_writeln("=== Tiny Operating System - User Space Shell ===");
    shell_writeln("Type 'help' for available commands.");
    shell_writeln("");
    
    /* Initialize current directory */
    if (syscall2(SYS_GETCWD, (int)current_directory, MAX_PATH_LEN) < 0) {
        strcpy(current_directory, "/");
    }
    
    /* Start main shell loop */
    shell_loop();
    
    /* Should never reach here */
    syscall0(SYS_EXIT);
}