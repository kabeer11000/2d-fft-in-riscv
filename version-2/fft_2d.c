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
