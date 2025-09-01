;
; Tiny Operating System - Boot Entry Point
; This is the assembly entry point that sets up the environment
; and calls the kernel main function
;

section .text
global _start

; Multiboot2 header for GRUB bootloader
align 8
mb2_header_start:
    dd 0xe85250d6               ; Magic number
    dd 0                        ; Architecture 0 (protected mode i386)
    dd mb2_header_end - mb2_header_start  ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (mb2_header_end - mb2_header_start))  ; Checksum
    
    ; End tag
    dw 0                        ; Type
    dw 0                        ; Flags
    dd 8                        ; Size
mb2_header_end:

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Clear BSS section
    mov edi, bss_start
    mov ecx, bss_end - bss_start
    xor eax, eax
    rep stosb
    
    ; Call kernel main function
    call kernel_main
    
    ; Infinite loop if kernel returns
    cli
.hang:
    hlt
    jmp .hang

; Stack section
section .bss
align 4
stack_bottom:
    resb 16384  ; 16KB stack
stack_top:

; BSS section
bss_start:
    resb 1
bss_end: