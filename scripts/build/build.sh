#!/bin/bash

CFLAGS="-rdynamic -O3 -march=x86-64 -g -std=c99 -Wall -Wextra -pedantic -I. -D_POSIX_C_SOURCE=200112L -Werror -Wno-unused-parameter"

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8
