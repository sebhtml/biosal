#!/bin/bash

# \see http://wiki.gentoo.org/wiki/GCC_optimization
# \see https://wiki.calculquebec.ca/w/GCC/en
CFLAGS="-O3 -march=x86-64 -fomit-frame-pointer -I. -static -m64 -flto -ffast-math "

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8

