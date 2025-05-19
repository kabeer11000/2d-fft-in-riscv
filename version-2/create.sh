#!/bin/bash

echo "Creating bare-metal RISC-V 2D FFT project files (Adding math and GCC runtime libs)..."

# --- 1. _start.s ---
cat << 'EOF' > _start.s
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
EOF
echo "Created _start.s"

# --- 2. uart.h ---
cat << 'EOF' > uart.h
#ifndef UART_H
#define UART_H

#include <stdint.h> // For uint8_t, uint64_t etc.

// Initialize the UART module
void uart_init(void);

// Transmit a single character
void uart_putc(char c);

// Transmit a null-terminated string
void uart_puts(const char* s);

// Transmit a hexadecimal number (64-bit)
void uart_puthex(uint64_t val);

// Transmit a floating-point number (basic, without full printf support)
void uart_putdouble(double val);

#endif // UART_H
EOF
echo "Created uart.h"

# --- 3. uart.c ---
cat << 'EOF' > uart.c
#include "uart.h"

// !!! IMPORTANT: Adjust this base address for your specific RISC-V board's UART !!!
// Common for SiFive Freedom E310/U540, or QEMU -M sifive_u
#define UART_BASE_ADDRESS 0x10000000

// UART Register Offsets (typical 16550-compatible UART)
#define UART_THR   (UART_BASE_ADDRESS + 0x00) // Transmit Holding Register
#define UART_LSR   (UART_BASE_ADDRESS + 0x05 * 4) // Line Status Register (Note: often offset by 4*register_index)

// Line Status Register bits
#define UART_LSR_TX_EMPTY (1 << 5) // Transmit Holding Register Empty

// Simple memory-mapped register access
#define UART_REG(reg) (*(volatile uint8_t *)(reg))

// Function to initialize UART (basic, assuming default configuration or handled externally)
void uart_init(void) {
    // For a real UART, you'd set baud rate, enable FIFOs, etc. here.
    // E.g., disable interrupts, set DLAB, write DLL/DLH, clear DLAB.
    // For simplicity, we assume a ready-to-use state.
}

// Transmit a single character
void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty
    while (!(UART_REG(UART_LSR) & UART_LSR_TX_EMPTY));
    // Write the character
    UART_REG(UART_THR) = c;
}

// Transmit a null-terminated string
void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Transmit a hexadecimal number (64-bit)
void uart_puthex(uint64_t val) {
    char hex_digits[] = "0123456789abcdef";
    char buf[16]; // For 16 hex digits (64-bit)
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = hex_digits[val % 16];
        val /= 16;
    }

    uart_puts("0x");
    for (i--; i >= 0; i--) {
        uart_putc(buf[i]);
    }
}

// Transmit a floating-point number (very basic, for demonstration)
// This implementation avoids full stdio.h dependency, but is limited.
// For bare-metal, full double printing is usually complex (requires custom dtoa).
// We'll print integer and fractional parts separately.
void uart_putdouble(double val) {
    if (val < 0) {
        uart_putc('-');
        val = -val;
    }

    uint64_t integer_part = (uint64_t)val;
    double fractional_part = val - integer_part;

    // Print integer part
    if (integer_part == 0) {
        uart_putc('0');
    } else {
        char buf[20]; // Max for 64-bit unsigned
        int i = 0;
        uint64_t temp = integer_part;
        while (temp > 0) {
            buf[i++] = (temp % 10) + '0';
            temp /= 10;
        }
        for (i--; i >= 0; i--) {
            uart_putc(buf[i]);
        }
    }

    uart_putc('.');

    // Print fractional part (e.g., 6 decimal places)
    for (int i = 0; i < 6; ++i) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        uart_putc(digit + '0');
        fractional_part -= digit;
    }
}
EOF
echo "Created uart.c"

# --- 4. fft_1d.h ---
cat << 'EOF' > fft_1d.h
#ifndef FFT_1D_H
#define FFT_1D_H

#include <complex.h> // For complex numbers

// Define a complex double type
typedef double complex cplx_double;

// Function to perform an in-place 1D FFT
// N: The size of the FFT (must be a power of 2)
// data: Pointer to the complex data array
// inverse: 0 for forward FFT, 1 for inverse FFT
void fft_1d(int N, cplx_double *data, int inverse);

#endif // FFT_1D_H
EOF
echo "Created fft_1d.h"

# --- 5. fft_1d.c ---
cat << 'EOF' > fft_1d.c
#include "fft_1d.h"
#include <math.h>     // For sin, cos, M_PI

// This section attempts to use RISC-V Vector Extension (V) if available.
// The actual vectorization of complex FFT is very challenging and may require
// working with real/imaginary parts separately or a dedicated RVV complex library.
// For now, the explicit RVV intrinsic calls are commented out to ensure compilation
// due to potential toolchain setup issues with specific intrinsic definitions.
// The code will fall back to scalar execution.
#ifdef __riscv_vector
#include <riscv_vector.h>
#else
// Dummy definitions if RVV is not enabled, for compilation purposes on non-RVV targets.
// These are minimal dummies and won't enable actual vectorization.
// For true RVV, you'd rely on the actual <riscv_vector.h> and specific types.
typedef double vfloat64m1_t;
typedef vfloat64m1_t vcomplex_double_t; // Dummy for complex
#define vsetvlmax_e64m1() 1
#define vfmv_v_f_f64m1(val, vl) (val)
#define vsetvli(vl, type, size) (vl)
#define RVV_E64 0 // Dummy
#define RVV_M1 0  // Dummy
#endif


// Function to perform bit reversal permutation
// This is typically done in-place.
// For large N, optimizing this with vector operations or parallel processing
// is possible but adds significant complexity.
static void bit_reverse_permutation(int N, cplx_double *data) {
    int i, j, k;
    for (i = 0, j = 0; i < N; ++i) {
        if (j > i) {
            cplx_double temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
        k = N >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

void fft_1d(int N, cplx_double *data, int inverse) {
    if (N <= 1) return;

    // Perform bit reversal permutation
    bit_reverse_permutation(N, data);

    // Iterative Cooley-Tukey algorithm
    for (int len = 2; len <= N; len <<= 1) { // len = current sub-transform size
        double angle = 2 * M_PI / len;
        if (inverse) angle = -angle; // For inverse FFT

        cplx_double wlen = cos(angle) + I * sin(angle); // Twiddle factor for this stage

        for (int i = 0; i < N; i += len) {
            cplx_double w = 1.0 + 0.0 * I; // Current twiddle factor

            // Butterfly operation: This loop is the core.
            // Vectorizing complex operations (especially multiplication) with RVV
            // directly on `complex double` is not straightforward with current intrinsics.
            // It often requires splitting real/imaginary parts into separate vectors
            // and manually performing complex multiplication using RVV floating-point operations.
            // For simplicity and correctness in a general bare-metal context,
            // we use a scalar loop. Compiler auto-vectorization might help for simple ops.
            for (int j = 0; j < len / 2; ++j) {
                cplx_double t = w * data[i + j + len / 2];
                data[i + j + len / 2] = data[i + j] - t;
                data[i + j] = data[i + j] + t;

                w *= wlen; // Update twiddle factor
            }
        }
    }

    // Scaling for inverse FFT
    if (inverse) {
        // --- RVV Vectorization Attempt for Scaling (Currently commented out for compilation) ---
        // This section demonstrates where RVV intrinsics *would* be used for a simple
        // element-wise operation. For `complex double`, this is more involved as it
        // requires processing real and imaginary parts separately with vector loads/stores.
        #ifdef __riscv_vector
        /*
        // Example of how RVV intrinsics might be used for scaling.
        // This is simplified and might need adjustment for actual complex number handling.
        size_t vlmax = vsetvlmax_e64m1(); // Maximum vector length for double (m1 group)
        double scale_factor = 1.0 / N;
        vfloat64m1_t v_scale_factor = vfmv_v_f_f64m1(scale_factor, vlmax); // Broadcast scalar

        for (size_t i = 0; i < N; ) {
            size_t vl = vsetvli(N - i, RVV_E64, RVV_M1); // Set vector length for remaining elements

            // To vectorize complex division, you'd typically load real and imaginary
            // parts into separate vector registers, perform division, and store back.
            // e.g.,
            // vfloat64m1_t v_real = vload_f64m1(&data[i].real, vl); // Hypothetical load for real parts
            // vfloat64m1_t v_imag = vload_f64m1(&data[i].imag, vl); // Hypothetical load for imag parts
            // v_real = vfdiv_vv_f64m1(v_real, v_scale_factor, vl);
            // v_imag = vfdiv_vv_f64m1(v_imag, v_scale_factor, vl);
            // vstore_f64m1(&data[i].real, v_real, vl); // Hypothetical store
            // vstore_f64m1(&data[i].imag, v_imag, vl); // Hypothetical store

            // For now, fallback to scalar loop for correctness and compilation:
            for (size_t k = 0; k < vl && (i + k) < N; ++k) {
                 data[i+k] /= N;
            }
            i += vl;
        }
        */
        // Scalar fallback for inverse scaling (always active for complex numbers)
        for (int i = 0; i < N; ++i) {
            data[i] /= N;
        }
        #else
        // Scalar fallback if RVV is not enabled or for complex types
        for (int i = 0; i < N; ++i) {
            data[i] /= N;
        }
        #endif
    }
}
EOF
echo "Created fft_1d.c"

# --- 6. fft_2d.h ---
cat << 'EOF' > fft_2d.h
#ifndef FFT_2D_H
#define FFT_2D_H

#include "fft_1d.h" // Includes cplx_double

// Function to perform an in-place 2D FFT
// rows: Number of rows in the 2D data (must be a power of 2)
// cols: Number of columns in the 2D data (must be a power of 2)
// data: Pointer to the complex 2D data array (row-major order)
// inverse: 0 for forward FFT, 1 for inverse FFT
void fft_2d(int rows, int cols, cplx_double *data, int inverse);

#endif // FFT_2D_H
EOF
echo "Created fft_2d.h"

# --- 7. fft_2d.c ---
cat << 'EOF' > fft_2d.c
#include "fft_2d.h"
#include "uart.h" // Include uart.h for error printing

// Define a maximum dimension for the FFT.
// This is used for the static transpose buffer.
// Setting it to a smaller, more manageable size (e.g., 32)
// to avoid "relocation truncated to fit" errors for now.
// If you need larger FFTs, you'll need dynamic memory allocation.
#define MAX_FFT_DIM 32
// The temporary buffer size for transpose will be MAX_FFT_DIM * MAX_FFT_DIM
// Ensure your BSS/data section can accommodate this.
static cplx_double temp_transpose_buffer[MAX_FFT_DIM * MAX_FFT_DIM];


void fft_2d(int rows, int cols, cplx_double *data, int inverse) {
    if (rows > MAX_FFT_DIM || cols > MAX_FFT_DIM) {
        // Only print error if UART is available (defined in uart.h)
        // This avoids errors if uart.h isn't included or UART_H isn't defined
        #ifdef UART_H
        uart_puts("Error: Image dimensions exceed MAX_FFT_DIM in fft_2d.c!\n");
        uart_puts("Please increase MAX_FFT_DIM or reduce image size.\n");
        #endif
        // In bare-metal, you might halt or return an error code.
        while(1); // Halt program if dimensions exceed buffer
    }

    // 1. Perform 1D FFT on each row
    for (int r = 0; r < rows; ++r) {
        fft_1d(cols, &data[r * cols], inverse);
    }

    // 2. Transpose the matrix
    // Copy data from 'data' (row-major) to 'temp_transpose_buffer' (column-major)
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            temp_transpose_buffer[c * rows + r] = data[r * cols + c];
        }
    }
    // Copy transposed data back to original array 'data'
    // This is effectively copying from temp_transpose_buffer (now in row-major order)
    // to data (also effectively row-major for next fft_1d call)
    for (int i = 0; i < rows * cols; ++i) {
        data[i] = temp_transpose_buffer[i];
    }

    // 3. Perform 1D FFT on each "new" row (which were original columns)
    // The matrix is now effectively transposed, so we process rows again using 'rows' as dimension.
    for (int c = 0; c < cols; ++c) { // Note: cols now represents the number of 1D FFTs to perform
        fft_1d(rows, &data[c * rows], inverse); // Each 1D FFT is of 'rows' length
    }

    // 4. Transpose back to original orientation (optional, depends on desired output)
    // If you need the output in the original row-major format.
    // Copy data from 'data' (column-major) to 'temp_transpose_buffer' (row-major)
    for (int r = 0; r < cols; ++r) { // Loop through "new" rows (original columns)
        for (int c = 0; c < rows; ++c) { // Loop through "new" columns (original rows)
            temp_transpose_buffer[c * cols + r] = data[r * rows + c];
        }
    }
    // Copy transposed data back to original array 'data'
    for (int i = 0; i < rows * cols; ++i) {
        data[i] = temp_transpose_buffer[i];
    }
}
EOF
echo "Created fft_2d.c"

# --- 8. main.c ---
cat << 'EOF' > main.c
#include "fft_2d.h"
#include "uart.h" // For bare-metal console output

// Define your image dimensions (must be powers of 2)
// These should be <= MAX_FFT_DIM defined in fft_2d.c
#define IMAGE_ROWS 16
#define IMAGE_COLS 16

// Statically allocate memory for the image data
// This will reside in the .bss section and be cleared by _start.s
static cplx_double image_data[IMAGE_ROWS * IMAGE_COLS];

// Function to print a complex number using the basic UART driver
void uart_print_complex(cplx_double val) {
    uart_putc('(');
    // Use GCC's __real__ and __imag__ extensions for bare-metal complex number access
    uart_putdouble(__real__(val));
    uart_putc(',');
    uart_putc(' ');
    uart_putdouble(__imag__(val));
    uart_putc(')');
}

// Main entry point for bare-metal
void main() {
    uart_init(); // Initialize your UART hardware

    uart_puts("2D FFT Example (Bare-Metal RISC-V)\n");
    uart_puts("----------------------------------\n");

    // Initialize image data (e.g., a simple square wave, or from external source)
    for (int r = 0; r < IMAGE_ROWS; ++r) {
        for (int c = 0; c < IMAGE_COLS; ++c) {
            // Example: A simple 2D pattern (checkerboard-like for some frequencies)
            if ((r < IMAGE_ROWS / 2 && c < IMAGE_COLS / 2) || (r >= IMAGE_ROWS / 2 && c >= IMAGE_COLS / 2)) {
                image_data[r * IMAGE_COLS + c] = 1.0 + 0.0 * I;
            } else {
                image_data[r * IMAGE_COLS + c] = 0.0 + 0.0 * I;
            }
        }
    }

    uart_puts("Original Image (first 5 elements):\n");
    for (int i = 0; i < 5; ++i) {
        uart_print_complex(image_data[i]);
        uart_puts(" ");
    }
    uart_putc('\n');


    // Perform Forward 2D FFT
    uart_puts("Performing Forward 2D FFT...\n");
    fft_2d(IMAGE_ROWS, IMAGE_COLS, image_data, 0); // 0 for forward FFT
    uart_puts("Forward 2D FFT Done.\n");

    uart_puts("FFT Output (first 5 elements, magnitude and phase):\n");
    for (int i = 0; i < 5; ++i) {
        uart_print_complex(image_data[i]);
        uart_puts(" ");
    }
    uart_putc('\n');

    // Perform Inverse 2D FFT
    uart_puts("Performing Inverse 2D FFT...\n");
    fft_2d(IMAGE_ROWS, IMAGE_COLS, image_data, 1); // 1 for inverse FFT
    uart_puts("Inverse 2D FFT Done.\n");

    uart_puts("Inverse FFT Output (first 5 elements, should be close to original):\n");
    for (int i = 0; i < 5; ++i) {
        uart_print_complex(image_data[i]);
        uart_puts(" ");
    }
    uart_putc('\n');

    uart_puts("Program Finished.\n");

    while (1) {
        // Infinite loop to keep the program running
    }
}
EOF
echo "Created main.c"

# --- 9. riscv_baremetal.ld ---
cat << 'EOF' > riscv_baremetal.ld
/* riscv_baremetal.ld - Linker script for bare-metal RISC-V rv64gcv */

/* Define the entry point of your program */
ENTRY(_start)

/* Define memory regions */
MEMORY {
    /*
     * RAM: Main system RAM.
     * Common starting address for many bare-metal RISC-V platforms (e.g., SiFive, QEMU).
     * !!! IMPORTANT: Adjust ORIGIN and LENGTH to match your specific board's RAM !!!
     */
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64M /* Example: 64 MB RAM */
}

SECTIONS {
    /*
     * .text.init section:
     * Contains the startup code (_start) and typically placed at the very beginning
     * of the executable memory region.
     */
    .text.init : {
        KEEP(*(.text.init)) /* Keep the _start symbol and its section */
        . = ALIGN(4);
    } > RAM

    /*
     * .text section:
     * Contains the main program code.
     */
    .text : {
        *(.text)            /* All other code */
        *(.text.*)
        . = ALIGN(4);       /* Ensure 4-byte alignment */
    } > RAM

    /*
     * .rodata section:
     * Read-only data, such as string literals and const variables.
     */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(8);       /* Align to 8 bytes for double/64-bit access */
    } > RAM

    /*
     * .data section:
     * Initialized data (global/static variables with initial values).
     * This data is loaded into RAM from the ELF file.
     */
    .data : {
        /*
         * Define __global_pointer$ here. It is traditionally set to the middle of
         * the small data section, but for bare-metal, a simple start of .data
         * or .sdata (if used) is often sufficient with -mno-relax.
         * The exact value isn't critical unless you explicitly use small data sections.
         */
        . = ALIGN(8);
        __global_pointer$ = .; /* Provide the global pointer symbol */

        *(.data)
        *(.data.*)
        . = ALIGN(8);       /* Align to 8 bytes for double/64-bit access */
    } > RAM

    /*
     * .bss section:
     * Uninitialized data (global/static variables without initial values).
     * This section is cleared to zeros by the startup code (_start.s).
     * It does not occupy space in the ELF file, only in RAM at runtime.
     */
    .bss : {
        . = ALIGN(8);       /* Align to 8 bytes for double/64-bit access */
        _bss_start = .;     /* Symbol for the beginning of BSS */
        *(.bss)
        *(.bss.*)
        *(COMMON)           /* Common uninitialized variables */
        . = ALIGN(8);
        _bss_end = .;       /* Symbol for the end of BSS */
    } > RAM

    /*
     * Stack and Heap:
     * These are typically managed by the program at runtime within the remaining RAM.
     * Stack grows downwards from _stack_top.
     * Heap (if implemented) grows upwards from _heap_start.
     */
    . = ALIGN(16);          /* Align for potential vector stack usage */
    _end = .;               /* Symbol for the end of used memory (start of heap) */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM); /* Stack starts at the very end of RAM */

    /* Ensure symbols are explicitly provided for external linkage */
    PROVIDE(_bss_start = _bss_start);
    PROVIDE(_bss_end = _bss_end);
    PROVIDE(_heap_start = _end); /* For custom heap implementation if needed */
    PROVIDE(_stack_top = _stack_top);
    PROVIDE(__global_pointer$ = __global_pointer$);
}
EOF
echo "Created riscv_baremetal.ld"

# --- 10. Makefile ---
cat << 'EOF' > Makefile
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
# -mcmodel=large: Explicitly set code model to large to address HI20 issues for larger binaries
CFLAGS = -nostdlib -nostartfiles -ffreestanding $(ARCH_FLAGS) -O2 -Wall -I. -D__riscv_vector -fno-builtin -fomit-frame-pointer -mno-relax -mcmodel=large

# --- Assembler Flags ---
ASFLAGS = $(ARCH_FLAGS) -mno-relax -mcmodel=large

# --- Linker Flags ---
# -T: Specify linker script
# -nostartfiles: Prevent default crt0.o from being linked (crucial for _start multiple definition)
# -nodefaultlibs: Prevents linking against standard libraries like libgcc, libc (we are bare-metal)
# -mno-relax: Matches compiler flag for consistent relocation handling
# Removed -mno-gpopt as it was unrecognized by your toolchain
# -mcmodel=large: Matches compiler flag for consistent code model
# -lm: Link with the math library (for sin, cos, etc.)
# -lgcc: Link with the GCC runtime support library (for __muldc3, etc.)
LDFLAGS = -T riscv_baremetal.ld $(ARCH_FLAGS) -nostartfiles -nodefaultlibs -mno-relax -mcmodel=large

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
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -lm -lgcc -o $@ # Link math and gcc runtime libraries here

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
EOF
echo "Created Makefile"

echo "All files generated successfully!"
echo "--------------------------------------------------------------------------------"
echo "### **Next Steps:**"
echo "1.  **Run the script:** `./create_project.sh` (this will regenerate all files)."
echo "2.  **Clean previous builds:** Run `make clean`."
echo "3.  **Compile the project:** Run `make` in this directory."
echo "    * The `R_RISCV_HI20` errors should be resolved by `-mcmodel=large`."
echo "    * The `undefined reference` errors for `sin`, `cos`, and `__muldc3` should be resolved by linking `-lm` and `-lgcc`."
echo "4.  **Review 'uart.c' and 'riscv_baremetal.ld' for your specific board.**"
echo "    -   Adjust **'UART_BASE_ADDRESS'** in 'uart.c'."
echo "    -   Adjust **'ORIGIN'** and **'LENGTH'** for **'RAM'** in 'riscv_baremetal.ld'."
echo "5.  **Run with QEMU (example):**"
echo "    `qemu-system-riscv64 -M sifive_u -bios none -kernel fft_2d_baremetal.elf -nographic -serial stdio`"
echo "    (Adjust `-M sifive_u` to your QEMU machine type if different, e.g., `-M virt`)"
echo "6.  **For real hardware:** Flash the `fft_2d_baremetal.bin` file to your board."
echo "--------------------------------------------------------------------------------"