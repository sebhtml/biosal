#!/bin/bash

CFLAGS="-O3 -march=native -I. "
clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8
