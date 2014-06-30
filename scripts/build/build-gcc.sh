#!/bin/bash

# \see http://wiki.gentoo.org/wiki/GCC_optimization
# \see https://wiki.calculquebec.ca/w/GCC/en
# in gcc 4.6 and later, -Ofast is -O3 + other goodies
CFLAGS="-Ofast -march=x86-64 -fomit-frame-pointer -I. -static -flto"

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8

