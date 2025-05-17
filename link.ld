ENTRY(_start)

MEMORY {
  RAM (rwx) : ORIGIN = 0x10000000, LENGTH = 512M
}

SECTIONS {
  . = ORIGIN(RAM);
  .text : { *(.text*) }         > RAM
  .rodata : { *(.rodata*) }     > RAM
  .data : { *(.data*) }         > RAM
  .bss : { *(.bss COMMON*) }     > RAM
}
