#!/bin/bash

#module swap PrgEnv-cray/4.2.24 PrgEnv-gnu/4.2.24
. /opt/modules/3.2.6.7/init/bash

module load PrgEnv-gnu/4.2.24
module load cray-mpich/6.0.2

make clean
make CC=cc -j 4 applications/argonnite_kmer_counter/argonnite CONFIG_FLAGS="-DTHORIUM_DEBUG" \
        applications/spate_metagenome_assembler/spate
