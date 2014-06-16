#!/bin/bash

#-Wall -std=c99
make clean
make -j 1 CC=clang CFLAGS="-I. -I/usr/include/mpich2 -D_POSIX_C_SOURCE=200112L -Wall --analyze -Werror -pedantic -x c -std=c99"
