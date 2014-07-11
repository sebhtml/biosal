#!/bin/bash

time mpiexec -n 4 applications/argonnite -print-counters -print-structure -k 43 -threads-per-node 7 ~/dropbox/medium-2.fastq | tee log
