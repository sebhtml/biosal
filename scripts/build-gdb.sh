#!/bin/bash

# build with -pg
# gprof Executable Gprof.data > Report

CFLAGS="-O0 -g -std=c99 -Wall -pedantic -I. -Werror -D_POSIX_C_SOURCE=200112L"

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8

