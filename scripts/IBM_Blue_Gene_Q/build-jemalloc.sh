#!/bin/bash

# These instructions are from:
# https://github.com/jeffhammond/HPCInfo/wiki/Malloc#Building_jemalloc_on_Blue_GeneQ
# and
# http://www.canonware.com/pipermail/jemalloc-discuss/2013-July/000617.html

cd $HOME/MALLOC
git clone https://github.com/jemalloc/jemalloc.git
cd jemalloc
./autogen.sh
./configure --host=powerpc64-bgq-linux --prefix=$HOME/MALLOC/jemalloc-install --disable-valgrind CC=powerpc64-bgq-linux-gcc \
    --with-jemalloc-prefix=jemalloc_

make CC="powerpc64-bgq-linux-gcc -Wl,-shared -shared" -j16
make CC="powerpc64-bgq-linux-gcc -Wl,-shared -shared" check
make install
