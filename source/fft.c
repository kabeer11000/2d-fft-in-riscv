#include "fft.h"
#include "image_data.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <riscv_vector.h>

#define PI 3.14159265358979323846

// Vector-optimized 1D FFT
void fft1d_vector(cplx* data, int n, int inverse) {
    if (n <= 1) return;

    // Split into even and odd
    cplx* even = (cplx*)malloc(n/2 * sizeof(cplx));
    cplx* odd = (cplx*)malloc(n/2 * sizeof(cplx));
    
    for (int i = 0; i < n/2; i++) {
        even[i] = data[2*i];
        odd[i] = data[2*i+1];
    }

    // Recursive calls
    fft1d_vector(even, n/2, inverse);
    fft1d_vector(odd, n/2, inverse);

    // Combine results using vector operations
    size_t vl;
    for (int k = 0; k < n/2; k += vl) {
        vl = vsetvlmax_e64m1();
        double angle = 2 * PI * k / n * (inverse ? 1 : -1);
        cplx w = cexp(I * angle);
        
        // Vector load
        vfloat64m1_t even_vec = vle64_v_f64m1((double*)&even[k], vl);
        vfloat64m1_t odd_vec = vle64_v_f64m1((double*)&odd[k], vl);
        
        // Complex multiplication with twiddle factor
        vfloat64m1_t result = vfmul_vf_f64m1(odd_vec, creal(w), vl);
        
        // Store results
        vse64_v_f64m1((double*)&data[k], vfadd_vv_f64m1(even_vec, result, vl), vl);
        vse64_v_f64m1((double*)&data[k + n/2], vfsub_vv_f64m1(even_vec, result, vl), vl);
    }

    free(even);
    free(odd);
}

// 2D FFT implementation
void fft2d_vector(cplx* data, int width, int height, int inverse) {
    // FFT on rows
    for (int i = 0; i < height; i++) {
        fft1d_vector(&data[i * width], width, inverse);
    }

    // FFT on columns
    cplx* col = (cplx*)malloc(height * sizeof(cplx));
    for (int j = 0; j < width; j++) {
        // Extract column
        for (int i = 0; i < height; i++) {
            col[i] = data[i * width + j];
        }
        
        // FFT on column
        fft1d_vector(col, height, inverse);
        
        // Store back
        for (int i = 0; i < height; i++) {
            data[i * width + j] = col[i];
        }
    }
    free(col);
}

// Image compression using FFT
void compress_image(Image* img, float threshold) {
    int size = img->width * img->height;
    cplx* fft_data = (cplx*)malloc(size * sizeof(cplx));
    
    // Convert image data to complex
    for (int i = 0; i < size; i++) {
        fft_data[i] = img->data[i];
    }
    
    // Perform 2D FFT
    fft2d_vector(fft_data, img->width, img->height, 0);
    
    // Apply threshold
    float max_magnitude = 0;
    for (int i = 0; i < size; i++) {
        float mag = cabs(fft_data[i]);
        if (mag > max_magnitude) max_magnitude = mag;
    }
    
    float threshold_value = max_magnitude * threshold;
    for (int i = 0; i < size; i++) {
        if (cabs(fft_data[i]) < threshold_value) {
            fft_data[i] = 0;
        }
    }
    
    // Inverse FFT
    fft2d_vector(fft_data, img->width, img->height, 1);
    
    // Convert back to image data
    for (int i = 0; i < size; i++) {
        img->data[i] = (uint8_t)fmin(255, fmax(0, creal(fft_data[i])));
    }
    
    free(fft_data);
}

// Save raw image data to file
void save_raw_image(const char* filename, uint8_t* data, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    fwrite(data, 1, width * height, f);
    fclose(f);
}

int main() {
    // Create image structure
    Image img = {
        .width = IMAGE_WIDTH,
        .height = IMAGE_HEIGHT,
        .data = malloc(IMAGE_WIDTH * IMAGE_HEIGHT)
    };
    
    // Copy embedded image data
    memcpy(img.data, image_data, IMAGE_WIDTH * IMAGE_HEIGHT);
    
    // Save original image
    save_raw_image("original.raw", img.data, img.width, img.height);
    
    // Compress image (keep 10% of frequency components)
    compress_image(&img, 0.1f);
    
    // Save compressed image
    save_raw_image("compressed.raw", img.data, img.width, img.height);
    
    // Cleanup
    free(img.data);
    return 0;
} 