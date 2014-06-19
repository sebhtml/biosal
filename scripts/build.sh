#!/bin/bash


# TODO qemu causes this with -march=native:
# test/interface.c:1:0: error: CPU you selected does not support x86-64 instruction set
# see http://pubs.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_02.html
# -Werror
# -rdynamic is to get the function names in glibc stack backtrace
#-Wconversion 
CFLAGS="-rdynamic -O3 -march=x86-64 -g -std=c99 -Wall -Wextra -pedantic -I. -D_POSIX_C_SOURCE=200112L -Werror -Wno-unused-parameter"
#-DBSAL_NODE_DEBUG -DBSAL_THREAD_DEBUG"

clear
echo "CFLAGS: $CFLAGS"

make clean
make CFLAGS="$CFLAGS" all -j 8

#make mock
#make test
#make ring
