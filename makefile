CXX = riscv64-linux-elf-gcc
CXXFLAGS = -O3 -march=rv64gcv -mabi=lp64d -std=c++17 -I.

all: http_fft

http_fft: http_fft.cpp lodepng.h link.ld
	$(CXX) $(CXXFLAGS) http_fft.cpp -o http_fft \
	  -static -Wl,-T,link.ld -lpthread

clean:
	rm -f http_fft
