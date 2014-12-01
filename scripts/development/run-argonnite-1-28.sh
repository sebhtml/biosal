#!/bin/bash

mpiexec -n 1 applications/argonnite_kmer_counter/argonnite -k 43 -print-thorium-data -threads-per-node 28 ~/dropbox/GPIC.1424-1.1371.fastq
