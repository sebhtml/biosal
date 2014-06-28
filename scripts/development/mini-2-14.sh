#!/bin/bash

mpiexec -n 2 applications/argonnite -print-load -k 43 -threads-per-node 14 ~/dropbox/mini.fastq | tee log
