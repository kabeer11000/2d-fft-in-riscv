// Dummy symbol for bare-metal environments
// Some standard library functions (like parts of libm) might reference __errno
// for error reporting (e.g., domain errors in math functions).
// In a -nostdlib bare-metal context, the standard C library that defines and
// manages 'errno' is not linked. Providing a dummy global variable named
// '__errno' satisfies the linker and allows library functions that have
// this dependency to be included from libm, even if they can't actually
// set a meaningful error number.

// Define the global variable __errno.
// It doesn't need to be initialized or used by your code,
// its mere existence satisfies the linker dependency.
int __errno;

// You might also need other symbols depending on the libraries you link.
// For instance, functions like `abort()` or `exit()` might be referenced
// by parts of the libraries even if you don't call them directly.
// Providing minimal stubs or looping functions for these can also be necessary.
// Example stub for abort (infinite loop):
/*
void abort() {
    while(1);
}
*/