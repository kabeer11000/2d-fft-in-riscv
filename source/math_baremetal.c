#include "riscv_vector.h"   
#include "math_baremetal.h"
#include <stdint.h> // For uint32_t

static float sin_table[TRIG_TABLE_SIZE];
static float cos_table[TRIG_TABLE_SIZE];

void init_trig_tables() {
    // As noted before, in true baremetal without a math lib, replace with precomputed data.
    // Assuming minimal sinf/cosf available during build/link phase.
    #ifndef NO_STDLIB_TRIG_INIT
    for (int i = 0; i < TRIG_TABLE_SIZE; ++i) {
        float angle = (float)i * 2.0f * M_PI_F / (float)TRIG_TABLE_SIZE;
        sin_table[i] = __builtin_sinf(angle); // or sinf(angle);
        cos_table[i] = __builtin_cosf(angle); // or cosf(angle);
    }
    #else
    #error "Define or provide sinf/cosf or replace init_trig_tables with precomputed data."
    #endif
}

static float get_table_value(const float* table, float rad) {
    while (rad < 0) rad += 2.0f * M_PI_F;
    while (rad >= 2.0f * M_PI_F) rad -= 2.0f * M_PI_F;
    float index_float = rad * (float)TRIG_TABLE_SIZE / (2.0f * M_PI_F);
    int index = (int)(index_float + 0.5f);
    if (index >= TRIG_TABLE_SIZE) index = 0;
    return table[index];
}

float baremetal_sin(float rad) { return get_table_value(sin_table, rad); }
float baremetal_cos(float rad) { return get_table_value(cos_table, rad); }
int is_power_of_two(unsigned int n) { return (n > 0) && ((n & (n - 1)) == 0); }
int log2_power_of_two(unsigned int n) {
     if (!is_power_of_two(n)) return -1;
     unsigned int count = 0;
     while (n > 1) { n >>= 1; count++; }
     return count;
}

// Basic bare-metal float to string. Limited precision.
// Does NOT handle NaN, Inf, or scientific notation.
int float_to_string(char* buf, int buf_size, float f, int decimal_places) {
    if (buf_size <= 0) return 0;

    char* ptr = buf;
    int written = 0;

    // Handle sign
    if (f < 0) {
        if (written < buf_size - 1) { *ptr++ = '-'; written++; } else { goto end; }
        f = -f;
    }

    // Handle integer part
    int int_part = (int)f;
    int divisor = 1;
    if (int_part == 0) {
         if (written < buf_size - 1) { *ptr++ = '0'; written++; } else { goto end; }
    } else {
        // Determine divisor for highest digit
        int temp = int_part;
        while(temp / 10 > 0) { divisor *= 10; temp /= 10; }

        while(divisor > 0) {
            if (written < buf_size - 1) { *ptr++ = (int_part / divisor) % 10 + '0'; written++; } else { goto end; }
            divisor /= 10;
        }
    }


    // Handle decimal part
    if (decimal_places > 0) {
        if (written < buf_size - 1) { *ptr++ = '.'; written++; } else { goto end; }

        float frac_part = f - (float)int_part;
        for (int i = 0; i < decimal_places; ++i) {
            frac_part *= 10;
            int digit = (int)frac_part;
            if (written < buf_size - 1) { *ptr++ = digit + '0'; written++; } else { goto end; }
            frac_part -= digit;
        }
    }

end:
    if (written < buf_size) { *ptr = '\0'; } // Null terminate if space
    return written; // Return characters written (excluding null terminator)
}