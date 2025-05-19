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
