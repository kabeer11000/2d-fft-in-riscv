.global _start      # Make _start visible to the linker
.section .text.init # Put this in a special section typically executed first

# --- Linker-defined symbols (defined in link.ld) ---
.extern __data_image # Address in ELF where initialized data resides (after rodata)
.extern __data_start # Address in RAM where initialized data should go
.extern __data_end   # Address in RAM where initialized data ends

.extern __bss_start  # Address in RAM where BSS starts
.extern __bss_end    # Address in RAM where BSS ends

.extern __stack_top  # Address for the top of the stack
# ---------------------------------------------------


_start:
    # 1. Initialize Data Section (.data)
    # Copy initialized data from ELF image to RAM
    la a0, __data_start  # Destination in RAM
    la a1, __data_end    # End of destination in RAM
    la a2, __data_image  # Source (initial values in ELF)

copy_data_loop:
    bge a0, a1, copy_data_end_loop # If destination >= end, finished copying
    lb a3, 0(a2)         # Load byte from source
    sb a3, 0(a0)         # Store byte at destination
    addi a0, a0, 1       # Increment destination pointer
    addi a2, a2, 1       # Increment source pointer
    j copy_data_loop
copy_data_end_loop:

    # 2. Zero BSS Section (.bss)
    # BSS contains uninitialized global/static variables, must be set to zero
    la a0, __bss_start   # Start of BSS
    la a1, __bss_end     # End of BSS
    li a2, 0             # Value to store (zero)

zero_bss_loop:
    bge a0, a1, zero_bss_end_loop # If start >= end, finished zeroing
    sb a2, 0(a0)         # Store byte 0 at current address
    addi a0, a0, 1       # Increment address
    j zero_bss_loop
zero_bss_end_loop:

    # 3. Set the Stack Pointer (sp)
    # Point the stack pointer to the top of the stack memory
    la sp, __stack_top   # Load Address: set stack pointer to __stack_top

    # 4. Call the main function
    call main

    # 5. If main returns (shouldn't in our case with while(1)), halt
    infinite_loop:
    j infinite_loop