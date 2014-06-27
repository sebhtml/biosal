#!/bin/bash

(
time mpiexec -n 2 applications/argonnite -print-load -k 43 -threads-per-node 14 ~/dropbox/GPIC.1424-1.1371.fastq
) &> log
