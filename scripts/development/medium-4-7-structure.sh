#!/bin/bash

mpiexec -n 4 applications/argonnite_kmer_counter/argonnite -print-counters -print-structure -k 43 -threads-per-node 7 ~/dropbox/medium.fastq
