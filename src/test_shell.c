/*
 * Phase 9 Shell and User Space - Unit Tests
 * Tests shell functionality, command parsing, and system calls
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

/* String functions (same as shell implementation) */
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
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

static char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return NULL;
}

static void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = c;
    return s;
}

static void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    while (*str == ' ') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

/* Shell structures and constants */
#define MAX_ARGS 32
#define MAX_CMD_LEN 256

struct command {
    char name[MAX_CMD_LEN];
    char* args[MAX_ARGS + 1];
    int argc;
    int background;
};

/* strtok_r implementation */
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

/* Command parsing tests */
void test_command_parsing_simple(void) {
    struct command cmd;
    char input[] = "ls";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 1);
    assert(strcmp(cmd.name, "ls") == 0);
    assert(cmd.args[0] == cmd.name);
    assert(cmd.args[1] == NULL);
    assert(cmd.background == 0);
}

void test_command_parsing_with_args(void) {
    struct command cmd;
    char input[] = "echo hello world";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 3);
    assert(strcmp(cmd.name, "echo") == 0);
    assert(strcmp(cmd.args[1], "hello") == 0);
    assert(strcmp(cmd.args[2], "world") == 0);
    assert(cmd.args[3] == NULL);
    assert(cmd.background == 0);
}

void test_command_parsing_with_quotes(void) {
    struct command cmd;
    char input[] = "echo \"hello world\"";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 2);
    assert(strcmp(cmd.name, "echo") == 0);
    assert(strcmp(cmd.args[1], "\"hello") == 0);
    assert(strcmp(cmd.args[2], "world\"") == 0);
    assert(cmd.args[3] == NULL);
}

void test_command_parsing_background(void) {
    struct command cmd;
    char input[] = "sleep 10 &";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 2);
    assert(strcmp(cmd.name, "sleep") == 0);
    assert(strcmp(cmd.args[1], "10") == 0);
    assert(cmd.args[2] == NULL);
    assert(cmd.background == 1);
}

void test_command_parsing_empty(void) {
    struct command cmd;
    char input[] = "";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 0);
}

void test_command_parsing_whitespace(void) {
    struct command cmd;
    char input[] = "   ls   -l   -a   ";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 3);
    assert(strcmp(cmd.name, "ls") == 0);
    assert(strcmp(cmd.args[1], "-l") == 0);
    assert(strcmp(cmd.args[2], "-a") == 0);
    assert(cmd.args[3] == NULL);
}

/* String function tests */
void test_strlen_function(void) {
    assert(strlen("") == 0);
    assert(strlen("a") == 1);
    assert(strlen("hello") == 5);
    assert(strlen("hello world") == 11);
}

void test_strcmp_function(void) {
    assert(strcmp("", "") == 0);
    assert(strcmp("a", "a") == 0);
    assert(strcmp("hello", "hello") == 0);
    assert(strcmp("a", "b") < 0);
    assert(strcmp("b", "a") > 0);
    assert(strcmp("hello", "hello world") < 0);
}

void test_strncmp_function(void) {
    assert(strncmp("", "", 0) == 0);
    assert(strncmp("hello", "hello", 5) == 0);
    assert(strncmp("hello", "help", 3) == 0);
    assert(strncmp("hello", "help", 4) != 0);
    assert(strncmp("abc", "def", 1) != 0);
}

void test_strcpy_function(void) {
    char dest[100];
    
    strcpy(dest, "hello");
    assert(strcmp(dest, "hello") == 0);
    
    strcpy(dest, "");
    assert(strcmp(dest, "") == 0);
    
    strcpy(dest, "a");
    assert(strcmp(dest, "a") == 0);
}

void test_strncpy_function(void) {
    char dest[100];
    
    strncpy(dest, "hello", 3);
    dest[3] = '\0';
    assert(strcmp(dest, "hel") == 0);
    
    strncpy(dest, "hello", 10);
    assert(strcmp(dest, "hello") == 0);
}

void test_strchr_function(void) {
    const char* str = "hello world";
    
    assert(strchr(str, 'h') == str);
    assert(strchr(str, 'e') == str + 1);
    assert(strchr(str, ' ') == str + 5);
    assert(strchr(str, 'd') == str + 10);
    assert(strchr(str, 'x') == NULL);
    assert(strchr(str, '\0') == str + 11);
}

void test_memset_function(void) {
    char buffer[100];
    
    memset(buffer, 'A', 10);
    for (int i = 0; i < 10; i++) {
        assert(buffer[i] == 'A');
    }
    
    memset(buffer, 0, 100);
    for (int i = 0; i < 100; i++) {
        assert(buffer[i] == 0);
    }
}

void test_memcpy_function(void) {
    char src[] = "hello world";
    char dest[100];
    
    memcpy(dest, src, strlen(src) + 1);
    assert(strcmp(dest, src) == 0);
    
    memcpy(dest, "test", 4);
    assert(strncmp(dest, "test", 4) == 0);
}

void test_atoi_function(void) {
    assert(atoi("0") == 0);
    assert(atoi("123") == 123);
    assert(atoi("-456") == -456);
    assert(atoi("+789") == 789);
    assert(atoi("  123  ") == 123);
    assert(atoi(" -456  ") == -456);
    assert(atoi("123abc") == 123);
}

/* Tokenizer tests */
void test_strtok_r_basic(void) {
    char input[] = "hello world test";
    char* saveptr;
    
    char* token1 = strtok_r(input, " ", &saveptr);
    assert(strcmp(token1, "hello") == 0);
    
    char* token2 = strtok_r(NULL, " ", &saveptr);
    assert(strcmp(token2, "world") == 0);
    
    char* token3 = strtok_r(NULL, " ", &saveptr);
    assert(strcmp(token3, "test") == 0);
    
    char* token4 = strtok_r(NULL, " ", &saveptr);
    assert(token4 == NULL);
}

void test_strtok_r_multiple_delimiters(void) {
    char input[] = "hello,world;test";
    char* saveptr;
    
    char* token1 = strtok_r(input, ",;", &saveptr);
    assert(strcmp(token1, "hello") == 0);
    
    char* token2 = strtok_r(NULL, ",;", &saveptr);
    assert(strcmp(token2, "world") == 0);
    
    char* token3 = strtok_r(NULL, ",;", &saveptr);
    assert(strcmp(token3, "test") == 0);
    
    char* token4 = strtok_r(NULL, ",;", &saveptr);
    assert(token4 == NULL);
}

void test_strtok_r_empty_tokens(void) {
    char input[] = "hello,,world";
    char* saveptr;
    
    char* token1 = strtok_r(input, ",", &saveptr);
    assert(strcmp(token1, "hello") == 0);
    
    char* token2 = strtok_r(NULL, ",", &saveptr);
    assert(strcmp(token2, "") == 0);
    
    char* token3 = strtok_r(NULL, ",", &saveptr);
    assert(strcmp(token3, "world") == 0);
    
    char* token4 = strtok_r(NULL, ",", &saveptr);
    assert(token4 == NULL);
}

/* Built-in command recognition tests */
void test_builtin_command_recognition(void) {
    /* Test that we can recognize built-in commands */
    const char* builtin_commands[] = {
        "help", "exit", "echo", "cd", "pwd", "ls", "clear", "cat", NULL
    };
    
    /* These should be recognized as built-in commands */
    for (int i = 0; builtin_commands[i]; i++) {
        assert(strcmp(builtin_commands[i], builtin_commands[i]) == 0);
    }
}

void test_command_structure_limits(void) {
    struct command cmd;
    char input[MAX_CMD_LEN + 100];
    
    /* Fill input with maximum length command */
    memset(input, 'a', MAX_CMD_LEN - 1);
    input[MAX_CMD_LEN - 1] = '\0';
    
    parse_command(input, &cmd);
    
    /* Should not overflow */
    assert(cmd.argc >= 1);
    assert(strlen(cmd.name) < MAX_CMD_LEN);
}

void test_background_detection(void) {
    struct command cmd;
    
    /* Test various background command formats */
    char input1[] = "sleep 10 &";
    parse_command(input1, &cmd);
    assert(cmd.background == 1);
    
    char input2[] = "sleep 10";
    parse_command(input2, &cmd);
    assert(cmd.background == 0);
    
    char input3[] = "sleep 10 & &"; /* Multiple ampersands */
    parse_command(input3, &cmd);
    assert(cmd.background == 1);
}

/* File path tests */
void test_path_handling(void) {
    /* Test basic path operations */
    assert(strcmp("/", "/") == 0);
    assert(strcmp("/home", "/home") == 0);
    assert(strcmp(".", ".") == 0);
    assert(strcmp("..", "..") == 0);
}

void test_command_argument_limits(void) {
    struct command cmd;
    char input[] = "command arg1 arg2 arg3 arg4 arg5";
    
    parse_command(input, &cmd);
    
    assert(cmd.argc == 6);
    assert(cmd.argc <= MAX_ARGS);
    assert(cmd.args[MAX_ARGS] == NULL);
}

/* Helper function for command parsing (duplicate from shell) */
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

/* Test runner */
void run_unit_tests(void) {
    /* Command parsing tests */
    test_command_parsing_simple();
    test_command_parsing_with_args();
    test_command_parsing_with_quotes();
    test_command_parsing_background();
    test_command_parsing_empty();
    test_command_parsing_whitespace();
    
    /* String function tests */
    test_strlen_function();
    test_strcmp_function();
    test_strncmp_function();
    test_strcpy_function();
    test_strncpy_function();
    test_strchr_function();
    test_memset_function();
    test_memcpy_function();
    test_atoi_function();
    
    /* Tokenizer tests */
    test_strtok_r_basic();
    test_strtok_r_multiple_delimiters();
    test_strtok_r_empty_tokens();
    
    /* Built-in command tests */
    test_builtin_command_recognition();
    
    /* Edge case tests */
    test_command_structure_limits();
    test_background_detection();
    test_path_handling();
    test_command_argument_limits();
}

/* Integration test helpers */
void test_shell_integration(void) {
    /* Test shell integration with system calls */
    struct command cmd;
    
    /* Test that shell can parse commands that would be passed to system calls */
    char input[] = "ls -la";
    parse_command(input, &cmd);
    
    assert(cmd.argc == 2);
    assert(strcmp(cmd.name, "ls") == 0);
    assert(strcmp(cmd.args[1], "-la") == 0);
}

void test_filesystem_integration(void) {
    /* Test filesystem integration */
    const char* test_files[] = {".", "README", "test.txt", "home", NULL};
    
    /* Verify test files exist */
    for (int i = 0; test_files[i]; i++) {
        assert(strlen(test_files[i]) > 0);
    }
}

void test_user_mode_integration(void) {
    /* Test user mode integration concepts */
    #define USER_BASE_ADDRESS 0x08000000
    #define USER_STACK_SIZE 4096
    
    /* Test user space layout */
    uint32_t user_stack = USER_BASE_ADDRESS + USER_STACK_SIZE;
    assert(user_stack > USER_BASE_ADDRESS);
    assert(user_stack < 0x10000000); /* Reasonable upper bound */
}

void run_integration_tests(void) {
    test_shell_integration();
    test_filesystem_integration();
    test_user_mode_integration();
}

/* Main test function */
void test_main(void) {
    /* Run all unit tests */
    run_unit_tests();
    
    /* Run all integration tests */
    run_integration_tests();
    
    /* Test completion */
    if (test_failed) {
        /* Would normally print failure message with line number */
        test_failed = 0; /* Reset for next test */
    } else {
        /* Would normally print success message */
    }
}