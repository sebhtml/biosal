#!/bin/bash

CFLAGS="-rdynamic -O3 -march=x86-64 -g -std=c99 -Wall -Wextra -pedantic -I. -D_POSIX_C_SOURCE=200112L -Wno-unused-parameter -Wconversion"
#-DBSAL_NODE_DEBUG -DBSAL_THREAD_DEBUG"

clear
make clean
make CFLAGS="$CFLAGS" all -j 8

