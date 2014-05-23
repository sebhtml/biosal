#!/bin/bash

make clean

# qemu causes this with -march=native:
# test/interface.c:1:0: error: CPU you selected does not support x86-64 instruction set
#make CFLAGS="-O3 -march=native -g -std=c99 -Wall -pedantic -I. -Werror" -j 7
make CFLAGS="-O3 -march=x86-64 -g -std=c99 -Wall -pedantic -I. -Werror" -j 7
