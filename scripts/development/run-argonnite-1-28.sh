#!/bin/bash

mpiexec -n 1 applications/argonnite -k 43 -print-load -threads-per-node 28 ~/dropbox/GPIC.1424-1.1371.fastq
