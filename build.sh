#!/bin/bash
riscv64-unknown-elf-as -march=rv64gcv vector.s -o vector.o
riscv64-unknown-elf-ld -T link.ld vector.o -o vector.elf
qemu-system-riscv64 -machine virt -cpu rv64,v=true -nographic -bios none -kernel vector.elf
