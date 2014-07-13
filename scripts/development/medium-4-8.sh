#!/bin/bash

mpiexec -n 4 applications/argonnite -print-memory-usage -print-load -k 43 -threads-per-node 8 ~/dropbox/medium.fastq
