#!/bin/bash

mpiexec -n 8 applications/spate_metagenome_assembler/spate -k 43 -threads-per-node 2 ~/dropbox/mini.fastq
