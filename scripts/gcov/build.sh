#!/bin/bash

# \see http://en.wikipedia.org/wiki/Gcov
CFLAGS="-Wall -fprofile-arcs -ftest-coverage -I."
clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8
