;
; Tiny Operating System - User Space System Call Handler
; Assembly handler for system calls with user space support
;

[bits 32]

; System call handler
global syscall_handler
extern syscall_handler_c
syscall_handler:
    ; Save registers
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10  ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Get system call number and arguments
    mov eax, [esp+36]  ; syscall number
    mov ebx, [esp+40]  ; arg1
    mov ecx, [esp+44]  ; arg2
    mov edx, [esp+48]  ; arg3
    mov esi, [esp+52]  ; arg4
    mov edi, [esp+56]  ; arg5
    
    ; Call C handler
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax
    call syscall_handler_c
    add esp, 24  ; Clean up arguments
    
    ; Restore registers
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    
    ; Return from interrupt
    iret

; Page fault handler
global page_fault_handler
extern page_fault_handler_c
page_fault_handler:
    ; Save registers
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10  ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Get error code and faulting address
    mov eax, [esp+36]  ; error code
    mov ebx, [esp+40]  ; EIP (we need CR2 for faulting address)
    
    ; Get CR2 (faulting address)
    mov ecx, cr2
    
    ; Call C handler
    push ecx
    push eax
    call page_fault_handler_c
    add esp, 8  ; Clean up arguments
    
    ; Restore registers
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    
    ; Return from interrupt
    iret