#!/bin/bash

echo "Running server..."

. ./litex-env/bin/activate

litex_sim --csr-json csr.json --cpu-type=vexriscv --cpu-variant=full --integrated-main-ram-size=0x06400000 --ram-init=boot.bin --with-ethernet --sys-clk-freq 100000000
