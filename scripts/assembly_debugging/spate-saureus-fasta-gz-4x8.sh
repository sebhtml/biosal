#!/bin/bash

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 33 -threads-per-node 8 ~/dropbox/S.aureus.fasta.gz \
    -aggregation-timeout 100000 \
