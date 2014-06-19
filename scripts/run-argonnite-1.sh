#!/bin/bash

time mpiexec -n 4 applications/argonnite -print-load -k 43 -threads-per-node 8 ~/dropbox/GPIC.1424-1.1371.fastq | tee log
