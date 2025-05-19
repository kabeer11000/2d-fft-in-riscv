#!/bin/bash

# Convert image to header
python3 convert_image.py

# Build the FFT program
make clean
make

# Run in QEMU
qemu-riscv64 ./fft_image

# Convert results to PNG
python3 view_results.py
