;
; Tiny Operating System - Simple User Program
; A simple user program to test system calls
;

[bits 32]

section .text
global _start

_start:
    ; Write "Hello, User Space!" using system call
    mov eax, 2           ; SYSCALL_WRITE
    mov ebx, 1           ; stdout
    mov ecx, message     ; message
    mov edx, 18          ; length
    int 0x80
    
    ; Get PID using system call
    mov eax, 12          ; SYSCALL_GETPID
    int 0x80
    
    ; Exit with success code
    mov eax, 0           ; SYSCALL_EXIT
    mov ebx, 0           ; exit code
    int 0x80

section .data
message db "Hello, User Space!", 10