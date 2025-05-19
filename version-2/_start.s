.section .text.init,"ax",@progbits
.global _start
_start:
    /* Set up global pointer (optional, comment out if not needed) */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    /* Set up stack pointer */
    .option push
    .option norelax
    lla sp, _stack_top
    .option pop

    /* Clear BSS section */
    la a0, _bss_start
    la a1, _bss_end
    bgeu a0, a1, 2f
1:
    sd zero, (a0)
    addi a0, a0, 8
    bltu a0, a1, 1b
2:

    call main

.loop_end:
    j .loop_end
