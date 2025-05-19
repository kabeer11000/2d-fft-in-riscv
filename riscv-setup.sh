# Set the installation path (replace with your desired location)
export RISCV=/opt/riscv

# Configure the build
./configure --prefix=$RISCV --enable-linux --enable-qemu-system --with-arch=rv64gc --with-abi=lp64d --with-languages=c,c++

# Build the toolchain and QEMU simulator
make -j$(nproc) all build-sim SIM=qemu
export PATH=$RISCV/bin:$PATH
