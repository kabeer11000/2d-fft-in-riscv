#ifndef COMPLEX_H
#define COMPLEX_H

#include "riscv_vector.h"

typedef struct {
    float real;
    float imag;
} complex_f32;

extern complex_f32 complex_add(complex_f32 a, complex_f32 b);
extern complex_f32 complex_sub(complex_f32 a, complex_f32 b);
extern complex_f32 complex_mul(complex_f32 a, complex_f32 b);

#endif // COMPLEX_H