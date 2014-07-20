#!/bin/bash

mpiexec -n 1 applications/argonnite -print-memory -print-load -k 43 -threads-per-node 32 ~/dropbox/GPIC.1424-1.1371.fastq
