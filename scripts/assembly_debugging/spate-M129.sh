#!/bin/bash

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 8 ~/dropbox/M129/*.fq -o M129-1 \
    -print-thorium-data
