#ifndef FFT_2D_H
#define FFT_2D_H

#include <complex.h>
#include <stdint.h>

typedef double complex cplx_double;

void fft_2d(int rows, int cols, cplx_double *data, int inverse);

#endif // FFT_2D_H
