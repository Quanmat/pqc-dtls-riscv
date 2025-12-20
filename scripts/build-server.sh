#!/bin/bash

echo "Cloning and building WolfSSL release for host to run example server..."

rm -rf wolfssl 2> /dev/null
git clone https://github.com/wolfSSL/wolfssl
cd wolfssl/
git checkout v5.8.4-stable
./autogen.sh
./configure --enable-dtls --enable-dtls13 --enable-kyber --enable-ipv6=no --enable-dtls-frag-ch --enable-dilithium
make
