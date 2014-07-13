#!/bin/bash

mpiexec -n 1 applications/argonnite -print-memory-usage -print-load -k 43 -threads-per-node 28 ~/dropbox/medium.fastq
