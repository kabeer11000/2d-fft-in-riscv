#ifndef FFT_1D_H
#define FFT_1D_H

#include <complex.h>
#include <stdint.h>

typedef double complex cplx_double;

void fft_1d(int N, cplx_double *data, int inverse);

#endif // FFT_1D_H
