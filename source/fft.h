#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <complex.h>

// Complex number type for FFT calculations
typedef double complex cplx;

// Structure to hold image data
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t* data;  // Grayscale image data
} Image;

// FFT functions
void fft2d(cplx* data, int width, int height, int inverse);
void fft1d(cplx* data, int n, int inverse);

// Image processing functions
Image* load_image(const char* filename);
void save_image(const char* filename, Image* img);
void compress_image(Image* img, float threshold);
void decompress_image(Image* img);

// Vector-optimized functions
void fft1d_vector(cplx* data, int n, int inverse);
void fft2d_vector(cplx* data, int width, int height, int inverse);

#endif // FFT_H 