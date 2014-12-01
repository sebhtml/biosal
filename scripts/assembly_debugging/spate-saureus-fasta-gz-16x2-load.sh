#!/bin/bash

    #-debug-memory-pools \

mpiexec -n 16 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 2 ~/dropbox/S.aureus.fasta.gz \
    -print-thorium-data \
    -enable-actor-load-profiler -enable-transport-profiler
