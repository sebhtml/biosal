#!/bin/bash

(
time mpiexec -n 1 applications/argonnite -print-load -k 43 -threads-per-node all ~/dropbox/GPIC.1424-1.1371.fastq
) &> log
