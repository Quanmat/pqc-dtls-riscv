#!/bin/bash

echo "Building openssl/liboqs + deps..."

PREFIX="$PWD"

# Dependencies required only for building (no sudo install)
sudo apt install -y build-essential git cmake ninja-build pkg-config \
    libssl-dev openssl python3-pip python3-yaml unzip doxygen graphviz \
    xsltproc valgrind astyle

# liboqs
rm -rf liboqs
git clone -b main https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir install
mkdir -p build && cd build
cmake -GNinja .. -DCMAKE_INSTALL_PREFIX="$PREFIX/liboqs/install" -DBUILD_SHARED_LIBS=ON
ninja
ninja install
cd ../..

# oqs-provider
rm -rf oqs-provider
git clone https://github.com/open-quantum-safe/oqs-provider.git
cd oqs-provider
mkdir -p install
mkdir -p _build
cd _build
cmake .. \
    -DCMAKE_INSTALL_PREFIX="$PREFIX/oqs-provider/install" \
    -DCMAKE_PREFIX_PATH="$PREFIX/liboqs/install" \
    -DOPENSSL_ROOT_DIR="/usr"
    -DOPENSSL_MODULES_PATH="$PREFIX/oqs-provider/install/lib/ossl-modules"
make -j$(nproc)
