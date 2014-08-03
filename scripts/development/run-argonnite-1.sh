#!/bin/bash

# change this from 8 threads to 7 threads because the machine is shared.
mpiexec -n 4 applications/argonnite_kmer_counter/argonnite -k 43 -threads-per-node 7 ~/dropbox/GPIC.1424-1.1371.fastq
