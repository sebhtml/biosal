#!/bin/bash

make clean

for i in tracepoints/lttng/*.tp
do
    lttng-gen-tp $i
done

#-std=c99
#
CFLAGS=" -O3 -march=x86-64 -g -I."

clear
echo "CFLAGS: $CFLAGS"

make CFLAGS="$CFLAGS" -j CONFIG_LTTNG=y CONFIG_DEBUG=n \
    applications/spate_metagenome_assembler/spate
