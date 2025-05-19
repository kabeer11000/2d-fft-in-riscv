/* riscv_baremetal.ld - Linker script for bare-metal RISC-V rv64gcv */

/* Define the entry point of your program */
ENTRY(_start)

/* Define memory regions */
MEMORY {
    /*
     * RAM: Main system RAM.
     * Common starting address for many bare-metal RISC-V platforms (e.g., SiFive, QEMU).
     * !!! IMPORTANT: Adjust ORIGIN and LENGTH to match your specific board's RAM !!!
     */
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64M /* Example: 64 MB RAM */
}

SECTIONS {
    /*
     * .text.init section:
     * Contains the startup code (_start) and typically placed at the very beginning
     * of the executable memory region.
     */
    .text.init : {
        KEEP(*(.text.init)) /* Keep the _start symbol and its section */
        . = ALIGN(4);
    } > RAM

    /*
     * .text section:
     * Contains the main program code.
     */
    .text : {
        *(.text)            /* All other code */
        *(.text.*)
        . = ALIGN(4);       /* Ensure 4-byte alignment */
    } > RAM

    /*
     * .rodata section:
     * Read-only data, such as string literals and const variables.
     */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(8);       /* Align to 8 bytes for double/64-bit access */
    } > RAM

    /*
     * .data section:
     * Initialized data (global/static variables with initial values).
     * This data is loaded into RAM from the ELF file.
     */
    .data : {
        /*
         * Define __global_pointer$ here. It is traditionally set to the middle of
         * the small data section, but for bare-metal, a simple start of .data
         * or .sdata (if used) is often sufficient with -mno-relax.
         * The exact value isn't critical unless you explicitly use small data sections.
         */
        . = ALIGN(8);
        __global_pointer$ = .; /* Provide the global pointer symbol */

        *(.data)
        *(.data.*)
        . = ALIGN(8);       /* Align to 8 bytes for double/64-bit access */
    } > RAM

    /*
     * .bss section:
     * Uninitialized data (global/static variables without initial values).
     * This section is cleared to zeros by the startup code (_start.s).
     * It does not occupy space in the ELF file, only in RAM at runtime.
     */
    .bss : {
        . = ALIGN(8);       /* Align to 8 bytes for double/64-bit access */
        _bss_start = .;     /* Symbol for the beginning of BSS */
        *(.bss)
        *(.bss.*)
        *(COMMON)           /* Common uninitialized variables */
        . = ALIGN(8);
        _bss_end = .;       /* Symbol for the end of BSS */
    } > RAM

    /*
     * Stack and Heap:
     * These are typically managed by the program at runtime within the remaining RAM.
     * Stack grows downwards from _stack_top.
     * Heap (if implemented) grows upwards from _heap_start.
     */
    . = ALIGN(16);          /* Align for potential vector stack usage */
    _end = .;               /* Symbol for the end of used memory (start of heap) */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM); /* Stack starts at the very end of RAM */

    /* Ensure symbols are explicitly provided for external linkage */
    PROVIDE(_bss_start = _bss_start);
    PROVIDE(_bss_end = _bss_end);
    PROVIDE(_heap_start = _end); /* For custom heap implementation if needed */
    PROVIDE(_stack_top = _stack_top);
    PROVIDE(__global_pointer$ = __global_pointer$);
}
