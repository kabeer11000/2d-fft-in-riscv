# Bare-Metal RISC-V "Hello World" on QEMU (Apple Silicon Compatible)

This project demonstrates how to compile and run a minimal RISC-V bare-metal program that prints `"Hello from RISC-V!"` using QEMU.

Tested on Apple Silicon with `qemu-system-riscv64` and `riscv64-unknown-elf-gcc`.

---

## Prerequisites

Make sure you have the following installed:

- QEMU with RISC-V support  
  Install via [Homebrew](https://brew.sh):
```bash
  brew install qemu
```

* RISC-V GCC toolchain (bare-metal, no OS):
  Install with Homebrew or from [SiFive](https://github.com/sifive/freedom-tools):

  ```bash
  brew tap riscv-software-src/riscv
  brew install riscv-tools
  ```

---

## Files

### `main.c`

```c
#define UART0_ADDR 0x10000000

// Inline assembly only — no C functions
asm (
    ".section .text\n"
    ".global _start\n"
    "_start:\n"
    "la a0, message\n"         // Load address of message
    "loop:\n"
    "lb a1, 0(a0)\n"           // Load byte
    "beqz a1, end\n"           // If zero, end
    "li a2, 0x10000000\n"      // UART0 address
    "sb a1, 0(a2)\n"           // Write byte to UART
    "addi a0, a0, 1\n"         // Next char
    "j loop\n"
    "end:\n"
    "j end\n"                  // Infinite loop
    ".align 4\n"
    "message:\n"
    ".asciz \"Hello from RISC-V!\\n\"\n"
);
```

---

### `link.ld`

```ld
ENTRY(_start)

MEMORY {
  RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
  . = ORIGIN(RAM);

  .text : { *(.text*) } > RAM
  .rodata : { *(.rodata*) } > RAM
  .data : { *(.data*) } > RAM
  .bss  : { *(.bss COMMON*) } > RAM
}
```

---

## Build

```bash
riscv64-unknown-elf-gcc -nostdlib -march=rv64g -mabi=lp64 \
  -T link.ld -o main.elf main.c
```

---

## Run in QEMU

```bash
qemu-system-riscv64 \
  -machine virt \
  -cpu rv64 \
  -nographic \
  -bios none \
  -kernel main.elf
```

---

## Expected Output

When you run the above command, you should see:

```
Hello from RISC-V!
```

printed in your terminal.

---

## 🧠 Notes

* We use inline assembly only (no C runtime or function calls).
* The program writes directly to UART at `0x10000000`, which is emulated by QEMU’s `virt` machine.
* This is **bare-metal** code — no operating system, no standard library.

