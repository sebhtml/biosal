#!/bin/bash

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 8 ~/dropbox/S.aureus.fasta.gz \
-enable-transport-profiler
