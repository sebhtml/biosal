#!/bin/bash

mpiexec -n 1 applications/spate_metagenome_assembler/spate -print-thorium-data -k 43 -threads-per-node 2 ~/dropbox/mini.fastq
