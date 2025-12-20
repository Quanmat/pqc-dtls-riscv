#!/bin/bash

echo "Running server..."

cd wolfssl
./examples/server/server -u -v 4 -p 4444 --pqc ML_KEM_512 -b -k ../boot/certs/server.key.pem -c ../boot/certs/server.pem -d
