/*
 * Tiny Operating System Linker Script
 * Defines memory layout for the kernel
 */

ENTRY(_start)

SECTIONS {
    /* Kernel starts at 1MB */
    . = 1M;
    
    /* Kernel code segment */
    .text : {
        *(.text)
        *(.rodata)
    }
    
    /* Kernel data segment */
    .data : {
        *(.data)
    }
    
    /* Kernel BSS segment */
    .bss : {
        *(.bss)
    }
    
    /* Discard sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}