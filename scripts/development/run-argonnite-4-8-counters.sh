#!/bin/bash

mpiexec -n 4 applications/argonnite -print-memory -print-load -print-counters -k 43 -threads-per-node 8 ~/dropbox/GPIC.1424-1.1371.fastq
