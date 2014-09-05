#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate -threads-per-node 8 \
    -k 55 ~/dropbox/SAMN02144067-Ecoli/* -o SAMN02144067-Ecoli-1
