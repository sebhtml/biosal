#!/bin/bash

mpiexec -n 2 applications/argonnite_kmer_counter/argonnite -print-thorium-data -k 43 -threads-per-node 14 ~/dropbox/mini.fastq
