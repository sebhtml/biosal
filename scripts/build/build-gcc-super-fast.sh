#!/bin/bash

CFLAGS="-O3 -march=native"
clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8
