#!/bin/bash

# TODO qemu causes this with -march=native:
# test/interface.c:1:0: error: CPU you selected does not support x86-64 instruction set
CFLAGS="-O3 -march=x86-64 -g -std=c99 -Wall -pedantic -I. -Werror -DBSAL_NODE_DEBUG1"

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8

make CFLAGS="$CFLAGS" mock
make CFLAGS="$CFLAGS" test
