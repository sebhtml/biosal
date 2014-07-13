#!/bin/bash

mpiexec -n 4 applications/argonnite -print-load -k 43 -threads-per-node 7 ~/dropbox/mini.fastq
