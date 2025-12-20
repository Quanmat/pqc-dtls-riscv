#!/bin/bash

echo "Starting setup..."

./scripts/litex-setup.sh
./scripts/openssl-setup.sh
./scripts/net-setup.sh
./scripts/gen-sim-files.sh
./scripts/build-server.sh

make
