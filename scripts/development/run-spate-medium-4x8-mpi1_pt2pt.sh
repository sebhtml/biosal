#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate \
  -transport mpi1_pt2pt_transport \
  -k 43 -threads-per-node 8 ~/dropbox/medium.fastq
