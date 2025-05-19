#include "fft_1d.h"
#include <math.h>     // For sin, cos, M_PI

// This section attempts to use RISC-V Vector Extension (V) if available.
// The actual vectorization of complex FFT is very challenging and may require
// working with real/imaginary parts separately or a dedicated RVV complex library.
// For now, the explicit RVV intrinsic calls are commented out to ensure compilation
// due to potential toolchain setup issues with specific intrinsic definitions.
// The code will fall back to scalar execution.
#ifdef __riscv_vector
#include <riscv_vector.h>
#else
// Dummy definitions if RVV is not enabled, for compilation purposes on non-RVV targets.
// These are minimal dummies and won't enable actual vectorization.
// For true RVV, you'd rely on the actual <riscv_vector.h> and specific types.
typedef double vfloat64m1_t;
typedef vfloat64m1_t vcomplex_double_t; // Dummy for complex
#define vsetvlmax_e64m1() 1
#define vfmv_v_f_f64m1(val, vl) (val)
#define vsetvli(vl, type, size) (vl)
#define RVV_E64 0 // Dummy
#define RVV_M1 0  // Dummy
#endif


// Function to perform bit reversal permutation
// This is typically done in-place.
// For large N, optimizing this with vector operations or parallel processing
// is possible but adds significant complexity.
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

    // Perform bit reversal permutation
    bit_reverse_permutation(N, data);

    // Iterative Cooley-Tukey algorithm
    for (int len = 2; len <= N; len <<= 1) { // len = current sub-transform size
        double angle = 2 * M_PI / len;
        if (inverse) angle = -angle; // For inverse FFT

        cplx_double wlen = cos(angle) + I * sin(angle); // Twiddle factor for this stage

        for (int i = 0; i < N; i += len) {
            cplx_double w = 1.0 + 0.0 * I; // Current twiddle factor

            // Butterfly operation: This loop is the core.
            // Vectorizing complex operations (especially multiplication) with RVV
            // directly on `complex double` is not straightforward with current intrinsics.
            // It often requires splitting real/imaginary parts into separate vectors
            // and manually performing complex multiplication using RVV floating-point operations.
            // For simplicity and correctness in a general bare-metal context,
            // we use a scalar loop. Compiler auto-vectorization might help for simple ops.
            for (int j = 0; j < len / 2; ++j) {
                cplx_double t = w * data[i + j + len / 2];
                data[i + j + len / 2] = data[i + j] - t;
                data[i + j] = data[i + j] + t;

                w *= wlen; // Update twiddle factor
            }
        }
    }

    // Scaling for inverse FFT
    if (inverse) {
        // --- RVV Vectorization Attempt for Scaling (Currently commented out for compilation) ---
        // This section demonstrates where RVV intrinsics *would* be used for a simple
        // element-wise operation. For `complex double`, this is more involved as it
        // requires processing real and imaginary parts separately with vector loads/stores.
        #ifdef __riscv_vector
        /*
        // Example of how RVV intrinsics might be used for scaling.
        // This is simplified and might need adjustment for actual complex number handling.
        size_t vlmax = vsetvlmax_e64m1(); // Maximum vector length for double (m1 group)
        double scale_factor = 1.0 / N;
        vfloat64m1_t v_scale_factor = vfmv_v_f_f64m1(scale_factor, vlmax); // Broadcast scalar

        for (size_t i = 0; i < N; ) {
            size_t vl = vsetvli(N - i, RVV_E64, RVV_M1); // Set vector length for remaining elements

            // To vectorize complex division, you'd typically load real and imaginary
            // parts into separate vector registers, perform division, and store back.
            // e.g.,
            // vfloat64m1_t v_real = vload_f64m1(&data[i].real, vl); // Hypothetical load for real parts
            // vfloat64m1_t v_imag = vload_f64m1(&data[i].imag, vl); // Hypothetical load for imag parts
            // v_real = vfdiv_vv_f64m1(v_real, v_scale_factor, vl);
            // v_imag = vfdiv_vv_f64m1(v_imag, v_scale_factor, vl);
            // vstore_f64m1(&data[i].real, v_real, vl); // Hypothetical store
            // vstore_f64m1(&data[i].imag, v_imag, vl); // Hypothetical store

            // For now, fallback to scalar loop for correctness and compilation:
            for (size_t k = 0; k < vl && (i + k) < N; ++k) {
                 data[i+k] /= N;
            }
            i += vl;
        }
        */
        // Scalar fallback for inverse scaling (always active for complex numbers)
        for (int i = 0; i < N; ++i) {
            data[i] /= N;
        }
        #else
        // Scalar fallback if RVV is not enabled or for complex types
        for (int i = 0; i < N; ++i) {
            data[i] /= N;
        }
        #endif
    }
}
