#!/bin/bash

mpiexec -n 1 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 28 ~/dropbox/S.aureus.fasta.gz
