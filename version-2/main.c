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
