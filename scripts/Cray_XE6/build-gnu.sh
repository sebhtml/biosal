#!/bin/bash

#module swap PrgEnv-cray/4.2.24 PrgEnv-gnu/4.2.24
. /opt/modules/3.2.6.7/init/bash

module purge

module load PrgEnv-gnu/4.2.24
module load cray-mpich/6.0.2
module load ugni/5.0-1.0402.7128.7.6.gem
module load xpmem/0.1-2.0402.44035.2.1.gem
module load udreg/2.3.2-1.0402.7311.2.1.gem

make clean
make CC=cc -j 4 applications/argonnite_kmer_counter/argonnite CONFIG_FLAGS="-DTHORIUM_DEBUG" \
        applications/spate_metagenome_assembler/spate
