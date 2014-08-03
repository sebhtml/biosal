#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate -print-load -k 43 -threads-per-node 8 ~/dropbox/medium.fastq
