#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "fft.h"
#include "image.h"
#include "compression.h"

#define PORT 8080
#define BUFFER_SIZE 8192 // For HTTP request/response

// Function prototypes
void handle_client(int client_sock);
int parse_http_request(char *request, char *method, char *uri, char **body_start, int *content_length);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;

    // 1. Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(PORT);

    // 2. Bind socket
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(server_sock, 5) == -1) { // 5 pending connections
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_addr_size = sizeof(client_addr);
        // 4. Accept a client connection
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock == -1) {
            perror("accept");
            continue; // Keep listening
        }
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_sock);
        close(client_sock); // Close client socket after handling
    }

    close(server_sock); // Never reached in this infinite loop
    return 0;
}

void handle_client(int client_sock) {
    char request_buffer[BUFFER_SIZE];
    char method[16], uri[256];
    char *body_start = NULL;
    int content_length = 0;
    ssize_t bytes_received;

    bytes_received = recv(client_sock, request_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        return;
    }
    request_buffer[bytes_received] = '\0'; // Null-terminate

    // Basic HTTP request parsing
    if (parse_http_request(request_buffer, method, uri, &body_start, &content_length) != 0) {
        // Simple error response
        const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nBad Request!\n";
        send(client_sock, response, strlen(response), 0);
        return;
    }

    printf("Method: %s, URI: %s, Content-Length: %d\n", method, uri, content_length);

    // Only handle POST to /compress
    if (strcmp(method, "POST") == 0 && strcmp(uri, "/compress") == 0) {
        if (body_start == NULL || content_length == 0) {
            const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 22\r\n\r\nMissing image data.\n";
            send(client_sock, response, strlen(response), 0);
            return;
        }

        // --- Image Processing Workflow ---
        // For simplicity, let's assume raw grayscale image data (e.g., 256x256 uint8_t)
        // You'd need to define your image format (e.g., width, height, pixel format)
        int width = 256; // Example
        int height = 256; // Example

        // This assumes the body directly contains the raw pixel data.
        // In a real scenario, you'd need to deserialize image data
        // (e.g., read width/height from the header, or a custom format).
        float *image_pixels_float = (float*)malloc(width * height * sizeof(float));
        if (!image_pixels_float) {
            perror("malloc");
            const char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 26\r\n\r\nMemory allocation failed.\n";
            send(client_sock, response, strlen(response), 0);
            return;
        }

        // Convert received byte data to float for FFT
        for (int i = 0; i < width * height; ++i) {
            // Assuming 8-bit grayscale for simplicity, scale to [0, 1] or [0, 255]
            image_pixels_float[i] = (float)((unsigned char)body_start[i]);
        }


        // 1. Perform 2D FFT
        // Output will be complex numbers (real and imaginary parts)
        float *fft_real = (float*)malloc(width * height * sizeof(float));
        float *fft_imag = (float*)malloc(width * height * sizeof(float));
        if (!fft_real || !fft_imag) {
            perror("malloc");
            const char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 26\r\n\r\nMemory allocation failed.\n";
            send(client_sock, response, strlen(response), 0);
            free(image_pixels_float); free(fft_real); free(fft_imag); // Free what's allocated
            return;
        }

        // Call the 2D FFT function (defined in fft.c)
        // This is a placeholder; a real FFT takes real input and produces complex output.
        // You'd need to handle input/output complex arrays.
        two_d_fft(image_pixels_float, fft_real, fft_imag, width, height);

        // 2. Apply Compression (Quantization + Simple Encoding)
        // This will be a very basic quantization for demonstration
        unsigned char *compressed_data = NULL;
        size_t compressed_size = 0;
        
        // This function will take fft_real and fft_imag, quantize, and encode.
        // Returns dynamically allocated compressed_data and its size.
        simple_compress(fft_real, fft_imag, width, height, &compressed_data, &compressed_size);

        if (!compressed_data || compressed_size == 0) {
            const char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 26\r\n\r\nCompression failed.\n";
            send(client_sock, response, strlen(response), 0);
            free(image_pixels_float); free(fft_real); free(fft_imag);
            return;
        }

        // --- Prepare HTTP Response ---
        char http_header[256];
        snprintf(http_header, sizeof(http_header),
                 "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %zu\r\n\r\n",
                 compressed_size);

        // Send header
        send(client_sock, http_header, strlen(http_header), 0);
        // Send compressed data
        send(client_sock, compressed_data, compressed_size, 0);

        // Clean up
        free(image_pixels_float);
        free(fft_real);
        free(fft_imag);
        free(compressed_data);

    } else {
        // Not found or unsupported method
        const char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 14\r\n\r\nNot Found!\n";
        send(client_sock, response, strlen(response), 0);
    }
}

// Very basic HTTP request parsing (only extracts method, URI, and Content-Length)
int parse_http_request(char *request, char *method, char *uri, char **body_start, int *content_length) {
    char *line = strtok(request, "\r\n");
    if (!line) return -1; // No request line

    // Parse request line (e.g., "POST /compress HTTP/1.1")
    if (sscanf(line, "%15s %255s HTTP/1.1", method, uri) != 2) {
        return -1; // Malformed request line
    }

    *content_length = 0;
    *body_start = NULL;

    // Parse headers
    while ((line = strtok(NULL, "\r\n")) != NULL && strlen(line) > 0) {
        if (strncmp(line, "Content-Length:", 15) == 0) {
            *content_length = atoi(line + 15);
        }
    }

    // The body starts after the double CRLF following the headers
    // This is a simplification; a robust parser would find the actual "\r\n\r\n" sequence.
    char *header_end = strstr(request, "\r\n\r\n");
    if (header_end) {
        *body_start = header_end + 4; // Move past "\r\n\r\n"
    }

    return 0;
}