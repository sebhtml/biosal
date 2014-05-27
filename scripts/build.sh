#!/bin/bash


# TODO qemu causes this with -march=native:
# test/interface.c:1:0: error: CPU you selected does not support x86-64 instruction set
# see http://pubs.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_02.html
CFLAGS="-O3 -march=x86-64 -g -std=c99 -Wall -pedantic -I. -Werror -D_POSIX_C_SOURCE=200112L"
#-DBSAL_NODE_DEBUG -DBSAL_THREAD_DEBUG"

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8

#make mock
#make test
#make ring
