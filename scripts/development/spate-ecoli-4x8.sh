#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate -threads-per-node 8 \
    -k 29 -p ~/dropbox/SRS213780-Ecoli/SRR306102_1.fastq.gz #~/dropbox/SRS213780-Ecoli/SRR306102_2.fastq.gz
