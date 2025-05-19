#include "fft_2d.h"
#include "fft_1d.h"
#include "uart.h"

#define MAX_FFT_DIM 16

static cplx_double temp_transpose_buffer[MAX_FFT_DIM * MAX_FFT_DIM];

void fft_2d(int rows, int cols, cplx_double *data, int inverse) {
    if (rows > MAX_FFT_DIM || cols > MAX_FFT_DIM) {
        uart_puts("Error: FFT dims too large!\n");
        while (1);
    }

    // FFT rows
    for (int r = 0; r < rows; ++r) {
        fft_1d(cols, &data[r * cols], inverse);
    }

    // Transpose
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            temp_transpose_buffer[c * rows + r] = data[r * cols + c];
        }
    }
    for (int i = 0; i < rows * cols; ++i) {
        data[i] = temp_transpose_buffer[i];
    }

    // FFT cols (now rows of transposed)
    for (int c = 0; c < cols; ++c) {
        fft_1d(rows, &data[c * rows], inverse);
    }

    // Transpose back
    for (int r = 0; r < cols; ++r) {
        for (int c = 0; c < rows; ++c) {
            temp_transpose_buffer[c * cols + r] = data[r * rows + c];
        }
    }
    for (int i = 0; i < rows * cols; ++i) {
        data[i] = temp_transpose_buffer[i];
    }
}
