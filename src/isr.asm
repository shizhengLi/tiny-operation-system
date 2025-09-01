;
; Tiny Operating System - Interrupt Service Routines
; Assembly handlers for interrupts and exceptions
;

[bits 32]

; Common ISR stub
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push 0      ; Push dummy error code
    push %1     ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push %1     ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Common IRQ stub
%macro IRQ 2
global irq%1
irq%1:
    cli
    push 0      ; Push dummy error code
    push %2     ; Push interrupt number
    jmp irq_common_stub
%endmacro

; Exception handlers (0-31)
ISR_NOERRCODE 0   ; Division by zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; Non-maskable interrupt
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Into detected overflow
ISR_NOERRCODE 5   ; Out of bounds
ISR_NOERRCODE 6   ; Invalid opcode
ISR_NOERRCODE 7   ; No coprocessor
ISR_ERRCODE    8   ; Double fault
ISR_NOERRCODE 9   ; Coprocessor segment overrun
ISR_ERRCODE    10  ; Bad TSS
ISR_ERRCODE    11  ; Segment not present
ISR_ERRCODE    12  ; Stack fault
ISR_ERRCODE    13  ; General protection fault
ISR_ERRCODE    14  ; Page fault
ISR_NOERRCODE 15  ; Unknown interrupt
ISR_NOERRCODE 16  ; Coprocessor fault
ISR_NOERRCODE 17  ; Alignment check
ISR_NOERRCODE 18  ; Machine check
ISR_NOERRCODE 19  ; SIMD floating point exception
ISR_NOERRCODE 20  ; Virtualization exception
ISR_NOERRCODE 21  ; Security exception
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Reserved
ISR_NOERRCODE 29  ; Reserved
ISR_NOERRCODE 30  ; Reserved
ISR_NOERRCODE 31  ; Reserved

; Hardware interrupt handlers (IRQ 0-15)
IRQ  0, 32    ; Timer
IRQ  1, 33    ; Keyboard
IRQ  2, 34    ; Cascade (used by PIC)
IRQ  3, 35    ; COM2
IRQ  4, 36    ; COM1
IRQ  5, 37    ; LPT2
IRQ  6, 37    ; Floppy disk
IRQ  7, 39    ; LPT1
IRQ  8, 40    ; CMOS real-time clock
IRQ  9, 41    ; Free for peripherals / SCSI
IRQ 10, 42    ; Free for peripherals / SCSI
IRQ 11, 43    ; Free for peripherals / SCSI
IRQ 12, 44    ; PS/2 mouse
IRQ 13, 45    ; FPU / coprocessor / Inter-processor
IRQ 14, 46    ; Primary ATA hard disk
IRQ 15, 47    ; Secondary ATA hard disk

; External C functions
extern isr_handler
extern irq_handler

; Common ISR stub
isr_common_stub:
    ; Save registers
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10  ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    push esp
    call isr_handler
    pop esp
    
    ; Restore registers
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret

; Common IRQ stub
irq_common_stub:
    ; Save registers
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10  ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    push esp
    call irq_handler
    pop esp
    
    ; Restore registers
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret