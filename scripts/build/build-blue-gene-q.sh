#!/bin/bash

# http://www-01.ibm.com/support/docview.wss?uid=swg27022103&aid=1

make clean
make -j 8 CFLAGS="-I. -O5 -s -qmaxmem=-1 -qarch=qp -qtune=qp " applications/argonnite
