.section .text.init,"ax",@progbits
.global _start
_start:
    /* Set up global pointer */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    /* Set up stack pointer (at the very top of RAM) */
    la sp, _stack_top

    /* Clear BSS section */
    la a0, _bss_start
    la a1, _bss_end
    bgeu a0, a1, 2f /* Skip if BSS is empty or invalid */
1:
    sd zero, (a0)   /* Store zero double-word (8 bytes) */
    addi a0, a0, 8  /* Increment pointer by 8 bytes */
    bltu a0, a1, 1b /* Loop until end of BSS */
2:

    /* Call the C entry point */
    call main

    /* Infinite loop after main returns (should not happen in bare-metal) */
.loop_end:
    j .loop_end
