# Define build directory and output file names
BUILD_DIR = build
ELF_FILE = $(BUILD_DIR)/hello.elf
MAIN_OBJ = $(BUILD_DIR)/main.o
START_OBJ = $(BUILD_DIR)/start.o
LINK_SCRIPT = link.ld

# Define source file names
MAIN_SRC = source/main.c
START_SRC = source/start.s

# Default target: build the final executable
.PHONY: default
default: $(ELF_FILE)

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule to compile the C source file
# Depends on the build directory existing
$(MAIN_OBJ): $(MAIN_SRC) | $(BUILD_DIR)
	riscv64-unknown-elf-gcc -c -g -O0 -ffreestanding -march=rv32i -mabi=ilp32 -o $@ $<

# Rule to assemble the assembly source file
# Depends on the build directory existing
$(START_OBJ): $(START_SRC) | $(BUILD_DIR)
	riscv64-unknown-elf-as -g -march=rv32i -mabi=ilp32 -o $@ $<

# Rule to link the object files into the final executable
# Depends on the object files and the linker script
$(ELF_FILE): $(MAIN_OBJ) $(START_OBJ) $(LINK_SCRIPT)
	riscv64-unknown-elf-ld -T $(LINK_SCRIPT) -m elf32lriscv -o $@ $(MAIN_OBJ) $(START_OBJ)

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
