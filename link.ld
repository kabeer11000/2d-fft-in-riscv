SECTIONS
{
  . = 0x80000000;													/* QEMU default load address to run bios */

  .text : {
		KEEP(*(.text._start));								/* Ensure _start is placed first */
		*(.text*);														/* Program code here */
	}

	. = ALIGN (CONSTANT (COMMONPAGESIZE));

  .rodata : { /* Read-only data */
    *(.rodata*)
  }

  .data : { /* Initialized data */
		*(.data*)
	}

  .bss (NOLOAD) : { /* Uninitialized data (buffers, global arrays) */
      . = ALIGN(32); /* Align for RVV access */
      __bss_start = .;
      *(.bss*)
      *(COMMON)
      . = ALIGN(32);
      __bss_end = .;
  }

  /* Stack is allocated space in the .data section based on start.s using .space */
  /* This is fine, stack grows downwards from __stack_top in .data */

  /* Define end of memory for clarity */
  . = ALIGN(4);
  __end = .;
}