#ifndef MATH_BAREMETAL_H
#define MATH_BAREMETAL_H

#include <stdint.h>

#define M_PI_F 3.14159265358979323846f
#define TRIG_TABLE_SIZE 256

void init_trig_tables();
float baremetal_sin(float rad);
float baremetal_cos(float rad);
int is_power_of_two(unsigned int n);
int log2_power_of_two(unsigned int n);

// Bare-metal float to string conversion
// Converts 'f' to string in 'buf' with 'decimal_places' after the point.
// Returns the number of characters written (excluding null terminator).
int float_to_string(char* buf, int buf_size, float f, int decimal_places);


#endif // MATH_BAREMETAL_H