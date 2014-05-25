#!/bin/bash

CFLAGS="-O3 -march=x86-64 -g -std=c99 -Wall -pedantic -I. -Werror -DBSAL_NODE_DEBUG1"

clear
echo "CFLAGS: $CFLAGS"

make clean
make all -j 8

make mock CFLAGS="$CFLAGS"
make test CFLAGS="$CFLAGS"
