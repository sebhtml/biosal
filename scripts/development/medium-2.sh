#!/bin/bash

time mpiexec -n 4 applications/argonnite -print-memory-usage -print-load -k 43 -threads-per-node 7 ~/dropbox/medium-2.fastq | tee log
