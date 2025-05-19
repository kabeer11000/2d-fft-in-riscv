# Makefile for Bare-Metal RISC-V 2D FFT

# --- Toolchain Configuration ---
# Adjust based on your RISC-V GNU Toolchain path and prefix
TOOLCHAIN_PREFIX = riscv64-unknown-elf-
CC = $(TOOLCHAIN_PREFIX)gcc
AS = $(TOOLCHAIN_PREFIX)as
LD = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy

# --- Architecture Flags ---
# As per your request: rv64gcv for architecture, lp64d for ABI
ARCH_FLAGS = -march=rv64gcv -mabi=lp64d

# --- Compiler Flags ---
# -nostdlib: No standard C library (we're bare-metal)
# -nostartfiles: No default startup files (we provide _start.s)
# -ffreestanding: Freestanding environment (no OS)
# -O2: Optimization level
# -Wall: Enable all warnings
# -I.: Add current directory to include paths
# -D__riscv_vector: Explicitly define for C code if needed (GCC usually defines it if -march includes 'v')
# -fno-builtin: Prevents GCC from using built-in functions that might rely on libc (e.g., memcpy, printf)
# -fomit-frame-pointer: Often used in embedded to save space
# -mno-relax: Crucial for RISC-V bare-metal to prevent certain linker optimizations that cause HI20 errors
# Removed -mno-gpopt as it was unrecognized by your toolchain
CFLAGS = -nostdlib -nostartfiles -ffreestanding $(ARCH_FLAGS) -O2 -Wall -I. -D__riscv_vector -mcmodel=large -fno-builtin -fomit-frame-pointer -mno-relax

# --- Assembler Flags ---
ASFLAGS = $(ARCH_FLAGS) -mno-relax 

# --- Linker Flags ---
# -T: Specify linker script
# -nostartfiles: Prevent default crt0.o from being linked (crucial for _start multiple definition)
# -nodefaultlibs: Prevents linking against standard libraries like libgcc, libc (we are bare-metal)
# -mno-relax: Matches compiler flag for consistent relocation handling
# Removed -mno-gpopt as it was unrecognized by your toolchain
LDFLAGS = -T riscv_baremetal.ld -lm -lgcc -mcmodel=large $(ARCH_FLAGS) -nostartfiles -nodefaultlibs -mno-relax

# --- Source Files ---
SRCS = main.c fft_1d.c fft_2d.c uart.c _start.s
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.s=.o) # Replace .s with .o as well

# --- Target Name ---
TARGET = fft_2d_baremetal

# --- Default Rule ---
all: $(TARGET).elf

# --- Compilation Rules ---
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

# --- Linking Rule ---
$(TARGET).elf: $(OBJS) riscv_baremetal.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

# --- Post-build Steps (Optional) ---
# Create a raw binary for flashing (e.g., for QEMU or actual hardware)
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Create a disassembly for inspection
$(TARGET).dis: $(TARGET).elf
	$(OBJCOPY) -O elf64-riscv --disassemble-all $< > $@ # Use elf64-riscv for 64-bit

# --- Clean Rule ---
clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).dis

.PHONY: all clean
