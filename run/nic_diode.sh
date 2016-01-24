#!/bin/sh

# The executable version of the shell script defined in nic_diode.run
sudo ip tuntap add dev tap-inner mode tap user $USER
sudo ip address add 10.0.11.1/24 dev tap-inner
sudo ip link set dev tap-inner up
sudo ip tuntap add dev tap-outer mode tap user $USER
sudo ip address add 10.0.66.1/24 dev tap-outer
sudo ip link set dev tap-outer up
