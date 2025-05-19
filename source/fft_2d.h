#ifndef FFT_2D_H
#define FFT_2D_H

#include "complex.h"
#include <stdint.h>

void fft_2d_rvv_f32(complex_f32 *data, int rows, int cols, int inverse);

#endif // FFT_2D_H