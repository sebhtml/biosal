#!/bin/bash

mpiexec -n 1 applications/argonnite -print-load -k 43 -threads-per-node 28 ~/dropbox/mini.fastq | tee log
