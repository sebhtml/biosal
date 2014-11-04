#!/bin/bash

module swap PrgEnv-cray/4.2.24 PrgEnv-pgi/4.2.24

make clean
make CC=cc -j 4 applications/argonnite_kmer_counter/argonnite CONFIG_DEBUG=y \
        applications/spate_metagenome_assembler/spate
