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
