#include "complex.h"

complex_f32 complex_add(complex_f32 a, complex_f32 b) {
    complex_f32 result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

complex_f32 complex_sub(complex_f32 a, complex_f32 b) {
    complex_f32 result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

complex_f32 complex_mul(complex_f32 a, complex_f32 b) {
    complex_f32 result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}