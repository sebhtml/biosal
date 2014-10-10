#!/bin/bash

CFLAGS="-fno-omit-frame-pointer -O3 -march=x86-64 -g -I."

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j
