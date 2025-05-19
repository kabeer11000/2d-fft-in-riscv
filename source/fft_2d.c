#include "fft_2d.h"
#include "fft_1d_rvv.h"
#include "complex.h"
#include <stdint.h>
#include <string.h> // For memcpy if needed

void fft_2d_rvv_f32(complex_f32 *data, int rows, int cols, int inverse) {
    // 1. Perform 1D FFT on each row
    for (int i = 0; i < rows; ++i) {
        fft_1d_rvv_f32(data + i * cols, cols, inverse);
    }

    // 2. Perform 1D FFT on each column using a temporary buffer
    complex_f32 temp_column[rows];

    for (int j = 0; j < cols; ++j) {
        for (int i = 0; i < rows; ++i) {
            temp_column[i] = data[i * cols + j];
        }

        fft_1d_rvv_f32(temp_column, rows, inverse);

        for (int i = 0; i < rows; ++i) {
            data[i * cols + j] = temp_column[i];
        }
    }
    // Scaling for IFFT is handled by the 1D FFT function.
}