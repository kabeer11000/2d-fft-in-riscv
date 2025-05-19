// Minimal putchar function for QEMU virt UART at 0x10000000
void my_putchar(char c) {
    volatile char *uart = (volatile char *)0x10000000;
    *uart = c;
}

// Minimal puts function
void my_puts(const char* s) {
    while (*s) {
        my_putchar(*s);
        s++;
    }
}
// Main entry point for the C code
int main() {
    my_puts("Hello, World!\n");

    // Halt execution in an infinite loop
    while(1);
    return 0;
}