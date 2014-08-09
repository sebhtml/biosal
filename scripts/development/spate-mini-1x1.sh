#!/bin/bash

mpiexec -n 1 applications/spate_metagenome_assembler/spate -k 43 -threads-per-node 1 ~/dropbox/mini.fastq
