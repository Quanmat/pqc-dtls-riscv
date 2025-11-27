```
git clone https://github.com/QTrino-Labs-Pvt-Ltd/C_Env_Sim.git
cd C_Env_Sim
```

```
python3 -m venv litex-env
source litex-env/bin/activate
```

```
chmod +x litex_setup.py
./litex_setup.py --init --install
pip3 install meson ninja
```

```
sudo ./litex_setup.py --gcc=riscv
sudo apt install libevent-dev libjson-c-dev verilator
```

```
litex_sim --csr-json csr.json --cpu-type=vexriscv --cpu-variant=full --uart-baudrate=1000000 --integrated-main-ram-size=0x06400000 --no-start
litex_bare_metal_demo --build-path=build/sim
litex_sim --csr-json csr.json --cpu-type=vexriscv --cpu-variant=full --uart-baudrate=1000000 --integrated-main-ram-size=0x06400000 --ram-init=boot.bin
```
