riscv64-unknown-linux-gcc -march=rv64gc -mabi=lp64d -O3 -Wall -Wextra \
    -I. \
    -o image_compress_server \
    server.c fft.c compression.c -lm