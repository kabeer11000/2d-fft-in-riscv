ENTRY(_start)

MEMORY {
  RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
  . = ORIGIN(RAM);

  .text : {
    KEEP(*(.text.init))   /* Keep our .text.init section (__start) */
    *(.text)              /* Regular text/code */
    *(.text.*)            /* Other text sections */
  } > RAM

  .rodata : {
    *(.rodata)            /* Read-only data (like our "Hello, World!\n" string) */
    *(.rodata.*)
  } > RAM

  .data : {
    . = ALIGN(8);
    __data_start = .;
    *(.data)
    *(.data.*)

    . = ALIGN(8);
    __data_end = .;
  } > RAM

  .bss : {
    . = ALIGN(8);
    __bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)

    . = ALIGN(8);
    __bss_end = .;
  } > RAM

  . = ALIGN(8);
  __data_image = .;

  . = 0x88000000;         /* Place stack high in RAM */
  __stack_top = .;

  /DISCARD/ : {
    *(.comment)
    *(.stab)
    *(.stabstr)
    *(.stab.hdr)
    *(.debug*)
    *(.gnu.attributes)
    *(.pdr)
  }
}