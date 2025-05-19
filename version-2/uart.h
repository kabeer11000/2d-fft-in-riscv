#ifndef UART_H
#define UART_H

#include <stdint.h> // For uint8_t, uint64_t etc.

// Initialize the UART module
void uart_init(void);

// Transmit a single character
void uart_putc(char c);

// Transmit a null-terminated string
void uart_puts(const char* s);

// Transmit a hexadecimal number (64-bit)
void uart_puthex(uint64_t val);

// Transmit a floating-point number (basic, without full uart_puts support)
void uart_putdouble(double val);

#endif // UART_H
