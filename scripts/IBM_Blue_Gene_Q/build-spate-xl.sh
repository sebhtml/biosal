#!/bin/bash

# http://www-01.ibm.com/support/docview.wss?uid=swg27022103&aid=1

# -O5 to enable -ipa
# -s to strip symbols

CFLAGS="-I. -g -O3 -qmaxmem=-1 -qarch=qp -qtune=qp -DTHORIUM_DEBUG "
CFLAGS="$CFLAGS -L/soft/libraries/alcf/current/xl/ZLIB/lib -I/soft/libraries/alcf/current/xl/ZLIB/include"

#echo $CFLAGS
#exit

make clean CONFIG_PAMI=y
make -j 8 CFLAGS="$CFLAGS" CONFIG_PAMI=y \
    applications/spate_metagenome_assembler/spate
