CXX = riscv64-unknown-elf-gcc
CXXFLAGS = -O3 -march=rv64gcv -mabi=lp64d -std=c11 -I.

all: fft_image

fft_image: source/fft.c source/fft.h link.ld
	$(CXX) $(CXXFLAGS) source/fft.c -o fft_image \
	  -static -Wl,-T,link.ld -lm

clean:
	rm -f fft_image

.PHONY: all clean
