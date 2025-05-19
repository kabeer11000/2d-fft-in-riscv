#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stddef.h> // For size_t

// Simple compression: quantize FFT coefficients and a very basic run-length encoding.
void simple_compress(float *fft_real, float *fft_imag, int width, int height, unsigned char **compressed_data, size_t *compressed_size);

#endif // COMPRESSION_H