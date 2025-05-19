#include "fft_1d.h"
#include "uart.h"
#include <math.h>

static void bit_reverse_permutation(int N, cplx_double *data) {
    int i, j, k;
    for (i = 0, j = 0; i < N; ++i) {
        if (j > i) {
            cplx_double temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
        k = N >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

void fft_1d(int N, cplx_double *data, int inverse) {
    if (N <= 1) return;

    bit_reverse_permutation(N, data);

    for (int len = 2; len <= N; len <<= 1) {
        double angle = 2 * M_PI / len;
        if (inverse) angle = -angle;

        cplx_double wlen = cos(angle) + I * sin(angle);

        for (int i = 0; i < N; i += len) {
            cplx_double w = 1.0 + 0.0 * I;
            for (int j = 0; j < len / 2; ++j) {
                cplx_double t = w * data[i + j + len / 2];
                data[i + j + len / 2] = data[i + j] - t;
                data[i + j] = data[i + j] + t;
                w *= wlen;
            }
        }
    }

    if (inverse) {
        for (int i = 0; i < N; ++i) {
            data[i] /= N;
        }
    }
}
