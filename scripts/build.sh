#!/bin/bash

CFLAGS="-O3 -march=x86-64 -g -std=c99 -Wall -pedantic -I. -Werror -DBSAL_NODE_DEBUG1"

make clean
make mock CFLAGS="$CFLAGS"

echo "TESTS"
make test CFLAGS="$CFLAGS"
