#!/bin/bash

mpiexec -n 1 applications/argonnite_kmer_counter/argonnite -print-load -k 43 -threads-per-node 28 ~/dropbox/medium.fastq \
-print-load
