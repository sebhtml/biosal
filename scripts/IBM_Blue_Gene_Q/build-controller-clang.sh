#!/bin/bash

CFLAGS="-I. -g -O3"
CFLAGS="$CFLAGS -I/soft/libraries/alcf/current/xl/ZLIB/include"
LDFLAGS="-L/soft/libraries/alcf/current/xl/ZLIB/lib -lm -lz"

#echo $CFLAGS
#exit

make clean
make -j 8 CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" CONFIG_DEBUG=y \
    examples/example_controller
