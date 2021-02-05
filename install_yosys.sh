#!/bin/bash

sudo apt-get install build-essential clang bison flex \
	libreadline-dev gawk tcl-dev libffi-dev git \
	graphviz xdot pkg-config python3 libboost-system-dev \
	libboost-python-dev libboost-filesystem-dev zlib1g-dev

git clone https://github.com/fpga-tool-org/yosys.git

cd yosys

make config-clang

mkdir build
cd build
make ../Makefile

sudo make install