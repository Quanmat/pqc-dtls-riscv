#!/bin/bash

echo "Setting up network (tap0) for litex..."

sudo ip tuntap add dev tap0 mode tap user $USER 2> /dev/null
sudo ip link set dev tap0 up
sudo ip addr add 192.168.1.100/24 dev tap0 2> /dev/null
sudo tc qdisc del dev tap0 root 2> /dev/null
sudo tc qdisc add dev tap0 root netem rate 100Kbit
