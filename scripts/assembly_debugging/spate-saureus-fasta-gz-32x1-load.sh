#!/bin/bash

    #-debug-memory-pools \

mpiexec -n 32 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 1 ~/dropbox/S.aureus.fasta.gz \
    -print-load \
    -enable-actor-load-profiler -enable-transport-profiler
