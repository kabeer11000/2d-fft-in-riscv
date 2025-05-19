#include "uart.h"

// !!! IMPORTANT: Adjust this base address for your specific RISC-V board's UART !!!
// Common for SiFive Freedom E310/U540, or QEMU -M sifive_u
#define UART_BASE_ADDRESS 0x10000000

// UART Register Offsets (typical 16550-compatible UART)
#define UART_THR   (UART_BASE_ADDRESS + 0x00) // Transmit Holding Register
#define UART_LSR   (UART_BASE_ADDRESS + 0x05 * 4) // Line Status Register (Note: often offset by 4*register_index)

// Line Status Register bits
#define UART_LSR_TX_EMPTY (1 << 5) // Transmit Holding Register Empty

// Simple memory-mapped register access
#define UART_REG(reg) (*(volatile uint8_t *)(reg))

// Function to initialize UART (basic, assuming default configuration or handled externally)
void uart_init(void) {
    // For a real UART, you'd set baud rate, enable FIFOs, etc. here.
    // E.g., disable interrupts, set DLAB, write DLL/DLH, clear DLAB.
    // For simplicity, we assume a ready-to-use state.
}

// Transmit a single character
void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty
    while (!(UART_REG(UART_LSR) & UART_LSR_TX_EMPTY));
    // Write the character
    UART_REG(UART_THR) = c;
}

// Transmit a null-terminated string
void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

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

// Transmit a floating-point number (very basic, for demonstration)
// This implementation avoids full stdio.h dependency, but is limited.
// For bare-metal, full double printing is usually complex (requires custom dtoa).
// We'll print integer and fractional parts separately.
void uart_putdouble(double val) {
    if (val < 0) {
        uart_putc('-');
        val = -val;
    }

    uint64_t integer_part = (uint64_t)val;
    double fractional_part = val - integer_part;

    // Print integer part
    if (integer_part == 0) {
        uart_putc('0');
    } else {
        char buf[20]; // Max for 64-bit unsigned
        int i = 0;
        uint64_t temp = integer_part;
        while (temp > 0) {
            buf[i++] = (temp % 10) + '0';
            temp /= 10;
        }
        for (i--; i >= 0; i--) {
            uart_putc(buf[i]);
        }
    }

    uart_putc('.');

    // Print fractional part (e.g., 6 decimal places)
    for (int i = 0; i < 6; ++i) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        uart_putc(digit + '0');
        fractional_part -= digit;
    }
}
