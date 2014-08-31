#!/bin/bash

# http://www-01.ibm.com/support/docview.wss?uid=swg27022103&aid=1

# -O5 to enable -ipa
# -s to strip symbols

CFLAGS="-I. -g -O3 -qmaxmem=-1 -qarch=qp -qtune=qp -DBSAL_DEBUGGER_ENABLE_ASSERT "
CFLAGS="$CFLAGS -L/soft/libraries/alcf/current/xl/ZLIB/lib -I/soft/libraries/alcf/current/xl/ZLIB/include"

#echo $CFLAGS
#exit

make clean
make -j 8 CFLAGS="$CFLAGS" CONFIG_PAMI=y \
    applications/argonnite_kmer_counter/argonnite \
    applications/spate_metagenome_assembler/spate \
    examples/example_ring
