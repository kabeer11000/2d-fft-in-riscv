#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h> // For uint8_t, uint16_t, etc.
#include <stddef.h> // For size_t

// --- Image Data Structure ---
// This structure will hold the raw pixel data and its dimensions.
// For simplicity in this project, we're assuming grayscale 8-bit images.
// You could extend this for RGB, different bit depths, etc.
typedef struct {
    int width;
    int height;
    // For grayscale 8-bit images (0-255).
    // This will hold the raw pixel values.
    uint8_t *pixels;
} ImageData;

// --- Function Prototypes (Optional, for more complex image handling) ---
// In this minimal server, we're handling raw pixels directly in server.c.
// If you wanted to abstract image loading/saving, you'd put prototypes here.

// Example: Function to allocate memory for image data
// ImageData* create_image_data(int width, int height);

// Example: Function to free image data
// void free_image_data(ImageData *img);

// Example: Basic image loading (e.g., from a raw binary file)
// ImageData* load_raw_grayscale_image(const char *filepath, int width, int height);

// Example: Basic image saving (e.g., to a raw binary file)
// int save_raw_grayscale_image(const char *filepath, const ImageData *img);


#endif // IMAGE_H