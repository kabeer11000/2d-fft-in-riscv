#include "fft_1d_rvv.h"
#include "complex.h"
#include "math_baremetal.h" // For sin/cos lookup and PI
#include <riscv_vector.h>   // RISC-V Vector Extension intrinsics
#include <string.h>         // Potentially for memcpy if needed

void bit_reverse_f32(complex_f32 *data, int n) {
    int i, j, k;
    j = 0;
    for (i = 0; i < n; ++i) {
        if (j > i) {
            complex_f32 temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
        k = n >> 1;
        while (k > 0 && (j & k)) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

void fft_1d_rvv_f32(complex_f32 *data, int n, int inverse) {
    bit_reverse_f32(data, n);

    for (int len = 2; len <= n; len <<= 1) {
        int half_len = len >> 1;
        float angle_scale = (inverse ? (2.0f * M_PI_F) : (-2.0f * M_PI_F)) / (float)len;

        for (int i = 0; i < n; i += len) {
            // Process butterflies in vector chunks
            for (int j = i; j < i + half_len; ) {
                size_t vl;
                // Set vector length based on remaining elements in the half-segment [i, i+half_len)
                // We process 'vl' pairs (data[j], data[j+half_len])
                vl = vsetvl_e32m1((i + half_len) - j); // Use m1 for 1 register per value (real/imag)

                // --- Load Twiddle Factors (Scalar Calculation -> Vector Load) ---
                // Calculate scalar W values for the current 'vl' elements (j, j+1, ..., j+vl-1)
                // and load them into vectors. This is the simpler bare-metal RVV approach.
                vfloat32m1_t v_w_real, v_w_imag;
                float w_reals[vl], w_imags[vl]; // Temporary scalar buffers

                for (size_t k_vec = 0; k_vec < vl; ++k_vec) {
                     int k_global = (j - i) + k_vec; // Overall index within the segment [0..half_len-1]
                     float angle = (float)k_global * angle_scale;
                     w_reals[k_vec] = baremetal_cos(angle);
                     w_imags[k_vec] = baremetal_sin(angle);
                }
                // Load the calculated scalar twiddle factors into vectors
                v_w_real = vle32_v_f32m1(w_reals, vl);
                v_w_imag = vle32_v_f32m1(w_imags, vl);
                // --- End Twiddle Factor Loading ---

                // Load data[j] (real/imaginary parts)
                vfloat32m1_t v_j_real = vle32_v_f32m1(&data[j].real, vl);
                vfloat32m1_t v_j_imag = vle32_v_f32m1(&data[j].imag, vl);

                // Load data[j + half_len] (real/imaginary parts)
                vfloat32m1_t v_jh_real = vle32_v_f32m1(&data[j + half_len].real, vl);
                vfloat32m1_t v_jh_imag = vle32_v_f32m1(&data[j + half_len].imag, vl);

                // --- Vector Complex Multiplication W * X[j + half_len] ---
                // (Wr + iWi) * (Xr + iXi) = (Wr*Xr - Wi*Xi) + i(Wr*Xi + Wi*Xr)

                // Calculate real part of product: Wr*Xr - Wi*Xi
                vfloat32m1_t term1_real = vfmul_vv_f32m1(v_w_real, v_jh_real, vl);
                vfloat32m1_t term2_real = vfmul_vv_f32m1(v_w_imag, v_jh_imag, vl);
                vfloat32m1_t product_real = vfsub_vv_f32m1(term1_real, term2_real, vl);

                // Calculate imaginary part of product: Wr*Xi + Wi*Xr
                vfloat32m1_t term1_imag = vfmul_vv_f32m1(v_w_real, v_jh_imag, vl);
                vfloat32m1_t term2_imag = vfmul_vv_f32m1(v_w_imag, v_jh_real, vl);
                vfloat32m1_t product_imag = vfadd_vv_f32m1(term1_imag, term2_imag, vl);

                // --- Vector Butterfly Operations ---
                // X'[j] = X[j] + product
                vfloat32m1_t v_j_new_real = vfadd_vv_f32m1(v_j_real, product_real, vl);
                vfloat32m1_t v_j_new_imag = vfadd_vv_f32m1(v_j_imag, product_imag, vl);

                // X'[j+half_len] = X[j] - product
                vfloat32m1_t v_jh_new_real = vfsub_vv_f32m1(v_j_real, product_real, vl);
                vfloat32m1_t v_jh_new_imag = vfsub_vv_f32m1(v_j_imag, product_imag, vl);

                // Store results back to memory
                vse32_v_f32m1(&data[j].real, v_j_new_real, vl);
                vse32_v_f32m1(&data[j].imag, v_j_new_imag, vl);
                vse32_v_f32m1(&data[j + half_len].real, v_jh_new_real, vl);
                vse32_v_f32m1(&data[j + half_len].imag, v_jh_new_imag, vl);

                // Advance loop counter
                j += vl;
            } // End of RVV butterfly loop for segment
        } // End of segment loop
    } // End of stages loop

    // If performing IFFT, scale by 1/n
    if (inverse) {
        float scale = 1.0f / (float)n;
        size_t vl;
        for (int i = 0; i < n; ) {
            vl = vsetvl_e32m1(n - i);
             vfloat32m1_t v_real = vle32_v_f32m1(&data[i].real, vl);
             vfloat32m1_t v_imag = vle32_v_f32m1(&data[i].imag, vl);

             v_real = vfmul_vf_f32m1(v_real, scale, vl);
             v_imag = vfmul_vf_f32m1(v_imag, scale, vl);

             vse32_v_f32m1(&data[i].real, v_real, vl);
             vse32_v_f32m1(&data[i].imag, v_imag, vl);
             i += vl;
        }
    }
}