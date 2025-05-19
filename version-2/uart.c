#include "uart.h"

#define UART_BASE_ADDRESS 0x10000000UL

// Offsets for 16550 UART with 8-bit spacing
#define UART_THR   (UART_BASE_ADDRESS + 0x00) // Transmit Holding Register
#define UART_LSR   (UART_BASE_ADDRESS + 0x05) // Line Status Register

#define UART_LSR_TX_EMPTY (1 << 5)

#define UART_REG(addr) (*(volatile uint8_t *)(addr))

void uart_init(void) {
    // QEMU UART is pre-configured — nothing to do
}

void uart_putc(char c) {
    while (!(UART_REG(UART_LSR) & UART_LSR_TX_EMPTY));
    UART_REG(UART_THR) = c;
}

void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// The rest of your uart_puthex and uart_putdouble are fine.

// Transmit a hexadecimal number (64-bit)
void uart_puthex(uint64_t val) {
    char hex_digits[] = "0123456789abcdef";
    char buf[16]; // For 16 hex digits (64-bit)
    int i = 0;

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0) {
        buf[i++] = hex_digits[val % 16];
        val /= 16;
    }

    uart_puts("0x");
    for (i--; i >= 0; i--) {
        uart_putc(buf[i]);
    }
}
void uart_putdouble(double val) {
    if (val < 0) {
        uart_putc('-');
        val = -val;
    }

    uint64_t integer_part = (uint64_t)val;
    double fractional_part = val - (double)integer_part;

    // Print integer part
    if (integer_part == 0) {
        uart_putc('0');
    } else {
        char buf[20];
        int i = 0;
        uint64_t temp = integer_part;
        while (temp > 0) {
            buf[i++] = (temp % 10) + '0';
            temp /= 10;
        }
        for (i = i - 1; i >= 0; i--) {
            uart_putc(buf[i]);
        }
    }

    uart_putc('.');

    // Print fractional part (6 digits)
    for (int i = 0; i < 6; ++i) {
        fractional_part *= 10.0;
        int digit = (int)fractional_part;
        uart_putc(digit + '0');
        fractional_part -= digit;
    }
}
