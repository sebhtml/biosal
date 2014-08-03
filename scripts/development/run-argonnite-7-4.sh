#!/bin/bash

mpiexec -n 7 applications/argonnite_kmer_counter/argonnite -print-load -k 43 -threads-per-node 4 ~/dropbox/GPIC.1424-1.1371.fastq
