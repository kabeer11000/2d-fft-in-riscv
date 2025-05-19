#include "compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // For fabs

// This is a VERY simplistic compression.
// A real image compression scheme (like JPEG) is much more sophisticated.
void simple_compress(float *fft_real, float *fft_imag, int width, int height, unsigned char **compressed_data, size_t *compressed_size) {
    // Quantization:
    // Scale FFT coefficients and round to integers.
    // Larger quantization_factor means more compression (more data loss).
    float quantization_factor = 100.0f; // Example: Adjust this value

    // For simplicity, we'll store quantized real and imaginary parts as short integers.
    // This is still very large, but shows the principle.
    short *quantized_coeffs = (short*)malloc(2 * width * height * sizeof(short)); // real and imag
    if (!quantized_coeffs) {
        perror("malloc for quantized_coeffs");
        *compressed_data = NULL;
        *compressed_size = 0;
        return;
    }

    int zero_count = 0;
    for (int i = 0; i < width * height; ++i) {
        quantized_coeffs[2 * i] = (short)roundf(fft_real[i] / quantization_factor);
        quantized_coeffs[2 * i + 1] = (short)roundf(fft_imag[i] / quantization_factor);

        if (quantized_coeffs[2 * i] == 0 && quantized_coeffs[2 * i + 1] == 0) {
            zero_count++;
        }
    }

    // Very basic "encoding":
    // For demonstration, we'll just copy the quantized data.
    // In a real scenario, you'd apply run-length encoding (RLE), Huffman coding,
    // or arithmetic coding to exploit the zeroes and distribution of coefficients.

    // A slightly better "encoding" for demonstration: simple run-length encoding for zeros.
    // This is still not robust or efficient for all cases.
    // Format: [value, count] or [0, zero_count]
    
    // For simplicity, let's just dump the quantized shorts.
    // This won't be very compressed unless many coefficients become 0.
    *compressed_size = 2 * width * height * sizeof(short);
    *compressed_data = (unsigned char*)malloc(*compressed_size);
    if (!*compressed_data) {
        perror("malloc for compressed_data");
        free(quantized_coeffs);
        *compressed_size = 0;
        return;
    }
    memcpy(*compressed_data, quantized_coeffs, *compressed_size);

    printf("Quantized coefficients: %d real, %d imag. Zero count: %d\n", width*height, width*height, zero_count);
    printf("Compressed size (raw shorts): %zu bytes\n", *compressed_size);

    free(quantized_coeffs);
}