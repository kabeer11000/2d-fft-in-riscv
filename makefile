# Define build directory and output file names
BUILD_DIR = build
ELF_FILE = $(BUILD_DIR)/hello.elf
# Add object files for new sources
MAIN_OBJ = $(BUILD_DIR)/main.o
START_OBJ = $(BUILD_DIR)/start.o
COMPLEX_OBJ = $(BUILD_DIR)/complex.o
MATH_OBJ = $(BUILD_DIR)/math_baremetal.o
FFT1D_OBJ = $(BUILD_DIR)/fft_1d_rvv.o
FFT2D_OBJ = $(BUILD_DIR)/fft_2d.o

LINK_SCRIPT = link.ld

# Define source file names
MAIN_SRC = source/main.c
START_SRC = source/start.s
COMPLEX_SRC = complex.c
MATH_SRC = math_baremetal.c
FFT1D_SRC = fft_1d_rvv.c
FFT2D_SRC = fft_2d.c
# Add the image data header as a dependency for main.c if needed, or assume it's generated
IMAGE_DATA_H = image_data.h # Main depends on this

# Define compiler/assembler/linker
GCC = riscv64-unknown-elf-gcc
AS = riscv64-unknown-elf-as
LD = riscv64-unknown-elf-ld

# Compiler flags:
# -c: Compile only, do not link
# -g: Include debug info
# -O0: No optimization (simplifies debugging)
# -ffreestanding: Compile for a freestanding environment (no standard library assumes math_baremetal provides needed functions)
# -march=rv32imacv: Specify architecture: RV32I + M (Mul/Div) + A (Atomics) + C (Compressed) + V (Vector)
#                   **ENSURE THIS MATCHES YOUR TARGET HARDWARE**
# -mabi=ilp32: Specify ABI
# -I. -I./source: Include current directory and source/ for headers
CFLAGS = -c -g -O0 -ffreestanding -march=rv32imacv -mabi=ilp32 -I. -I./source
AFLAGS = -g -march=rv32imacv -mabi=ilp32
LFLAGS = -T $(LINK_SCRIPT) -m elf32lriscv

# All object files
OBJS = $(MAIN_OBJ) $(START_OBJ) $(COMPLEX_OBJ) $(MATH_OBJ) $(FFT1D_OBJ) $(FFT2D_OBJ)

# Default target: build the final executable
.PHONY: default
default: $(ELF_FILE)

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generic rule for compiling .c files to .o (for files in the current directory)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(GCC) $(CFLAGS) -o $@ $<

# Rule for compiling main.c (specific source path, depends on image data header)
$(MAIN_OBJ): $(MAIN_SRC) $(IMAGE_DATA_H) | $(BUILD_DIR)
	$(GCC) $(CFLAGS) -o $@ $<

# Rule for assembling start.s (specific source path)
$(START_OBJ): $(START_SRC) | $(BUILD_DIR)
	$(AS) $(AFLAGS) -o $@ $<

# Rule to link the object files into the final executable
# Depends on all object files and the linker script
$(ELF_FILE): $(OBJS) $(LINK_SCRIPT)
	$(LD) $(LFLAGS) -o $@ $(OBJS)

# Rule to run the executable using QEMU
.PHONY: run
run: $(ELF_FILE)
	@echo "Ctrl-A C for QEMU console, then quit to exit"
	qemu-system-riscv32 -nographic -serial mon:stdio -machine virt -bios $(ELF_FILE)

# Rule to debug the executable using QEMU
.PHONY: debug
debug: $(ELF_FILE)
	@echo "Ctrl-A C for QEMU console, then quit to exit"
	qemu-system-riscv32 -nographic -serial mon:stdio -machine virt -s -S -bios $(ELF_FILE)

# Rule to clean up build files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)