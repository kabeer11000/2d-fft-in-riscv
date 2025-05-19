#define UART0_BASE 0x10000000

// Use a datasheet for a 16550 UART
// For example: https://www.ti.com/lit/ds/symlink/tl16c550d.pdf
#define REG(base, offset) ((*((volatile unsigned char *)(base + offset))))
#define UART0_DR REG(UART0_BASE, 0x00)
#define UART0_FCR REG(UART0_BASE, 0x02)
#define UART0_LSR REG(UART0_BASE, 0x05)

#define UARTFCR_FFENA 0x01 // UART FIFO Control Register enable bit
#define UARTLSR_THRE 0x20  // UART Line Status Register Transmit Hold Register Empty bit
#define UART0_FF_THR_EMPTY (UART0_LSR & UARTLSR_THRE)

void uart_putc(char c)
{
    while (!UART0_FF_THR_EMPTY)
        ;         // Wait until the FIFO holding register is empty
    UART0_DR = c; // Write character to transmitter register
}

void uart_puts(const char *str)
{
    while (*str)
    {                      // Loop until value at string pointer is zero
        uart_putc(*str++); // Write the character and increment pointer
    }
}

// Include image data and FFT headers
#include "image_data.h"
#include "complex.h"
#include "math_baremetal.h" // For init_trig_tables and float_to_string
#include "fft_2d.h"

// Allocate buffer for complex data. Placed in .bss by default for uninitialized globals.
// Use alignment attribute for potential RVV performance boost.
static complex_f32 fft_buffer[IMAGE_WIDTH * IMAGE_HEIGHT] __attribute__((aligned(32)));

// Bare metal not traditional C/C++ entry point
void main()
{
    UART0_FCR = UARTFCR_FFENA;   // Set the FIFO for polled operation
    uart_puts("System starting...\n");

    // Initialize bare-metal math tables
    init_trig_tables();
    uart_puts("Trig tables initialized.\n");

    // --- Prepare Input Data ---
    // Copy real image data into the complex buffer, setting imaginary parts to 0.
    if (sizeof(input_image_data) / sizeof(input_image_data[0]) != IMAGE_WIDTH * IMAGE_HEIGHT) {
        uart_puts("Error: Image data size mismatch!\n");
        while(1); // Halt on error
    }

    for(int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; ++i) {
        fft_buffer[i].real = input_image_data[i];
        fft_buffer[i].imag = 0.0f; // Input is real-valued
    }
    uart_puts("Input data copied to buffer.\n");

    // --- Perform 2D FFT ---
    uart_puts("Performing 2D FFT...\n");
    // Check if dimensions are powers of 2 (basic check)
    if (!is_power_of_two(IMAGE_WIDTH) || !is_power_of_two(IMAGE_HEIGHT)) {
         uart_puts("Error: Image dimensions must be power of 2!\n");
         while(1); // Halt on error
    }

    fft_2d_rvv_f32(fft_buffer, IMAGE_HEIGHT, IMAGE_WIDTH, 0); // 0 for forward FFT
    uart_puts("2D FFT complete.\n");

    // --- Output Result over UART ---
    uart_puts("Outputting FFT data:\n");

    char float_str_buf[20]; // Buffer for float-to-string, adjust size as needed (e.g. for 6 decimal places)
    int total_elements = IMAGE_WIDTH * IMAGE_HEIGHT;
    int elements_per_line = 8; // Control how many values per line for readability

    for(int i = 0; i < total_elements; ++i) {
        // Output Real part
        float_to_string(float_str_buf, sizeof(float_str_buf), fft_buffer[i].real, 4); // 4 decimal places
        uart_puts(float_str_buf);
        uart_putc(','); // Comma separator

        // Output Imaginary part
        float_to_string(float_str_buf, sizeof(float_str_buf), fft_buffer[i].imag, 4); // 4 decimal places
        uart_puts(float_str_buf);

        // Add comma unless it's the very last element
        if (i < total_elements - 1) {
            uart_putc(',');
        }

        // Add newline periodically for readability, or if it's the last element
        if ((i + 1) % elements_per_line == 0 || i == total_elements - 1) {
             uart_putc('\n');
        }
    }
     uart_puts("\nOutput complete.\n");


    while (1)
        ; // Loop forever
}