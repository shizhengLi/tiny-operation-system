;
; Tiny Operating System - MBR Bootloader
; This is a real-mode bootloader that loads the kernel from disk
; and switches to protected mode
;

[org 0x7C00]    ; BIOS loads bootloader at 0x7C00
[bits 16]       ; Start in 16-bit real mode

section .text
global start

start:
    jmp main             ; Jump to main code
    nop                 ; Padding for BIOS parameter block

; BIOS Parameter Block (BPB) - simplified for floppy disk
bpb:
    OEMName:            db "TINYOS  "   ; 8 bytes
    BytesPerSector:     dw 512          ; Bytes per sector
    SectorsPerCluster:  db 1            ; Sectors per cluster
    ReservedSectors:    dw 1            ; Reserved sectors (including bootloader)
    NumberOfFATs:       db 2            ; Number of FAT copies
    RootEntries:        dw 224          ; Number of root directory entries
    TotalSectors:       dw 2880         ; Total sectors (1.44MB floppy)
    MediaDescriptor:    db 0xF0         ; Media descriptor
    SectorsPerFAT:      dw 9            ; Sectors per FAT
    SectorsPerTrack:    dw 18           ; Sectors per track
    NumberOfHeads:      dw 2            ; Number of heads
    HiddenSectors:      dd 0            ; Hidden sectors
    TotalSectorsBig:    dd 0            ; Total sectors big (if > 65535)
    DriveNumber:        db 0            ; Drive number
    Reserved:           db 0            ; Reserved
    Signature:          db 0x29         ; Extended signature
    VolumeID:           dd 0x12345678   ; Volume ID
    VolumeLabel:        db "TINY OS     " ; Volume label (11 bytes)
    FileSystemType:     db "FAT12   "   ; File system type (8 bytes)

main:
    ; Save drive number
    mov [DriveNumber], dl
    
    ; Initialize segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Set stack below bootloader
    
    ; Clear screen
    call clear_screen
    
    ; Print welcome message
    mov si, boot_msg
    call print_string
    
    ; Read kernel from disk
    call load_kernel
    
    ; Check if kernel loaded successfully
    cmp ax, 0
    jne kernel_load_error
    
    ; Switch to protected mode
    call enable_protected_mode
    
    ; Jump to kernel
    jmp 0x08:0x10000  ; Jump to kernel at physical address 0x10000

; Function: clear_screen
; Clears the VGA text mode screen
clear_screen:
    pusha
    mov ax, 0x0600      ; Scroll entire screen
    mov bx, 0x0700      ; Attribute (light gray on black)
    mov cx, 0x0000      ; Upper left corner
    mov dx, 0x184F      ; Lower right corner
    int 0x10            ; BIOS video interrupt
    popa
    ret

; Function: print_string
; Input: SI = string address
print_string:
    pusha
    mov ah, 0x0E        ; BIOS teletype function
.print_loop:
    lodsb               ; Load next character
    cmp al, 0           ; Check for null terminator
    je .done
    int 0x10            ; Print character
    jmp .print_loop
.done:
    popa
    ret

; Function: load_kernel
; Loads kernel from disk to memory at 0x10000
load_kernel:
    pusha
    mov si, load_msg
    call print_string
    
    ; Read kernel sectors (assume kernel starts at sector 2, size 20 sectors)
    mov ax, 0x1000      ; Destination segment (0x10000)
    mov es, ax
    xor bx, bx          ; Destination offset (0)
    
    mov ah, 0x02        ; BIOS read function
    mov al, 20          ; Number of sectors to read
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Sector 2 (sector 1 is bootloader)
    mov dh, 0           ; Head 0
    mov dl, [DriveNumber] ; Drive number
    int 0x13
    
    jc .read_error     ; Check for read error
    
    ; Verify read success
    cmp al, 20          ; Check if all sectors read
    jne .read_error
    
    mov si, success_msg
    call print_string
    xor ax, ax          ; Success
    jmp .done
    
.read_error:
    mov si, error_msg
    call print_string
    mov ax, 1           ; Error code
.done:
    popa
    ret

; Function: enable_protected_mode
; Switches CPU from real mode to protected mode
enable_protected_mode:
    pusha
    mov si, pmode_msg
    call print_string
    
    ; Disable interrupts
    cli
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Enable A20 line
    call enable_a20
    
    ; Set PE bit in CR0
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    
    ; Far jump to flush CPU pipeline
    jmp 0x08:protected_mode_entry

; Function: enable_a20
; Enables the A20 address line
enable_a20:
    pusha
    ; Try BIOS method first
    mov ax, 0x2401
    int 0x15
    
    ; If BIOS method fails, try keyboard controller method
    jc .keyboard_method
    jmp .done
    
.keyboard_method:
    call wait_keyboard_input
    mov al, 0xAD        ; Disable keyboard
    out 0x64, al
    
    call wait_keyboard_input
    mov al, 0xD0        ; Read from input port
    out 0x64, al
    
    call wait_keyboard_output
    in al, 0x60        ; Read current value
    push ax            ; Save it
    
    call wait_keyboard_input
    mov al, 0xD1        ; Write to output port
    out 0x64, al
    
    call wait_keyboard_input
    pop ax             ; Get saved value
    or al, 0x02        ; Set A20 bit
    out 0x60, al
    
    call wait_keyboard_input
    mov al, 0xAE        ; Enable keyboard
    out 0x64, al
    
    call wait_keyboard_input
.done:
    popa
    ret

wait_keyboard_input:
    in al, 0x64
    test al, 0x02
    jnz wait_keyboard_input
    ret

wait_keyboard_output:
    in al, 0x64
    test al, 0x01
    jz wait_keyboard_output
    ret

; Protected mode entry point
[bits 32]
protected_mode_entry:
    ; Set up protected mode segments
    mov ax, 0x10        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov esp, 0x90000    ; Stack at top of loaded kernel
    
    ; Jump to kernel entry point
    jmp 0x10000  ; Jump to kernel at physical address 0x10000

; Global Descriptor Table (GDT)
gdt_start:
    ; Null segment (required)
    dd 0
    dd 0
    
    ; Code segment (0x08)
    dw 0xFFFF          ; Limit (0-15)
    dw 0x0000          ; Base (0-15)
    db 0x00            ; Base (16-23)
    db 10011010b       ; Access byte
    db 11001111b       ; Flags + Limit (16-19)
    db 0x00            ; Base (24-31)
    
    ; Data segment (0x10)
    dw 0xFFFF          ; Limit (0-15)
    dw 0x0000          ; Base (0-15)
    db 0x00            ; Base (16-23)
    db 10010010b       ; Access byte
    db 11001111b       ; Flags + Limit (16-19)
    db 0x00            ; Base (24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size
    dd gdt_start                ; GDT address

; Error handling
kernel_load_error:
    mov si, kernel_error_msg
    call print_string
    jmp $

; Messages
boot_msg: db 'Tiny OS Bootloader', 13, 10, 0
load_msg: db 'Loading kernel...', 13, 10, 0
success_msg: db 'OK', 13, 10, 0
error_msg: db 'Error!', 13, 10, 0
pmode_msg: db 'Protected mode...', 13, 10, 0
kernel_error_msg: db 'Kernel failed!', 13, 10, 0

; Bootloader signature (required by BIOS)
times 510-($-$$) db 0   ; Pad to 510 bytes
dw 0xAA55               ; Boot signature (2 bytes)