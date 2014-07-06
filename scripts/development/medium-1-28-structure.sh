#!/bin/bash

time mpiexec -n 1 applications/argonnite -print-structure -k 43 -threads-per-node 28 ~/dropbox/medium-2.fastq | tee log
