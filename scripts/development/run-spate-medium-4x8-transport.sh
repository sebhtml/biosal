#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate -k 43 -threads-per-node 8 \
    -enable-transport-profiler ~/dropbox/medium.fastq
