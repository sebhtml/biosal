#!/bin/bash

# http://www-01.ibm.com/support/docview.wss?uid=swg27022103&aid=1

# -O5 to enable -ipa
# -s to strip symbols

CFLAGS="-I. -g -O3 -qmaxmem=-1 -qarch=qp -qtune=qp "
CFLAGS="$CFLAGS -L/soft/libraries/alcf/current/xl/ZLIB/lib -I/soft/libraries/alcf/current/xl/ZLIB/include"

make clean CONFIG_PAMI=y
make -j 8 CFLAGS="$CFLAGS" CONFIG_PAMI=y CONFIG_DEBUG=n LDFLAGS="-L/soft/perftools/mpiP/lib/ -lmpiP -liberty" \
        performance/latency_probe/latency_probe

