#include "uart.h"
#include "fft_2d.h"

int main(void) {
    uart_init();
    uart_puts("Starting 2D FFT demo...\n");

    // Example 4x4 matrix of complex numbers (real only for demo)
    #define N 4
    uart_puts("Original data:\n");
    cplx_double data[N * N] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    };

    for (int i = 0; i < N * N; ++i) {
        uart_putdouble(creal(data[i]));
        uart_putc(' ');
        uart_putdouble(cimag(data[i]));
        uart_puts("i\n");
    }

    fft_2d(N, N, data, 0);

    uart_puts("FFT result:\n");
    for (int i = 0; i < N * N; ++i) {
        uart_putdouble(creal(data[i]));
        uart_putc(' ');
        uart_putdouble(cimag(data[i]));
        uart_puts("i\n");
    }

    uart_puts("Done.\n");
    while (1);

    return 0;
}
