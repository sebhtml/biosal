#!/bin/bash

CFLAGS="-I. -g -O3 -DTHORIUM_DEBUG "
CFLAGS="$CFLAGS -I/soft/libraries/alcf/current/xl/ZLIB/include"
LDFLAGS="-L/soft/libraries/alcf/current/xl/ZLIB/lib -lm -lz"

#echo $CFLAGS
#exit

make clean
make -j 8 CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
    applications/spate_metagenome_assembler/spate
