#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate -threads-per-node 8 \
    -k 19 ~/dropbox/SAMN02743420-Ecoli-P5C3/SRR1284073.fasta \
    -o spate-Ecoli-P5C3-1
