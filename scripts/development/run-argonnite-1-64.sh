#!/bin/bash

mpiexec -n 1 applications/argonnite -print-load -print-memory-usage -k 43 -threads-per-node 64 ~/dropbox/GPIC.1424-1.1371.fastq
