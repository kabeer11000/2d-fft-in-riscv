#ifndef FFT_1D_RVV_H
#define FFT_1D_RVV_H

#include "complex.h"
#include <stdint.h>

void fft_1d_rvv_f32(complex_f32 *data, int n, int inverse);

#endif // FFT_1D_RVV_H