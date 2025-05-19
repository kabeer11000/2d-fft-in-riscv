#ifndef COMPLEX_H
#define COMPLEX_H

typedef struct {
    float real;
    float imag;
} complex_f32;

complex_f32 complex_add(complex_f32 a, complex_f32 b);
complex_f32 complex_sub(complex_f32 a, complex_f32 b);
complex_f32 complex_mul(complex_f32 a, complex_f32 b);

#endif // COMPLEX_H