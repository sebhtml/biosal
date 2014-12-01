#!/bin/bash

    #-debug-memory-pools \

mpiexec -n 1 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 32 ~/dropbox/S.aureus.fasta.gz \
    -print-thorium-data \
    -enable-actor-load-profiler -enable-transport-profiler
