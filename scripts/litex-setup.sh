#!/bin/bash

echo "Setting up litex..."

rm -rf litex-env 2> /dev/null
python3 -m venv litex-env
source litex-env/bin/activate
chmod +x litex_setup.py
./litex_setup.py --init --install
pip3 install meson ninja
sudo ./litex_setup.py --gcc=riscv
sudo apt install libevent-dev libjson-c-dev verilator -y
