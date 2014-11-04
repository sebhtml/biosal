#!/bin/bash

# see http://jlse.anl.gov/installed-software/

# use Intel MPI which will pull the Intel compiler which
# supports cross-compiling.
#source /soft/compilers/intel/impi/4.1.3.048/bin64/mpivars.sh
# This is the x86-64 compiler...

export INTEL_LICENSE_FILE=28518@ftsn2
source /soft/compilers/intel/composer_xe_2013_sp1.1.106/bin/compilervars.sh

make clean
make -j CFLAGS="-mmic -O3 -I." CC=icc
