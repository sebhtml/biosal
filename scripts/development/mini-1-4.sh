#!/bin/bash

mpiexec -n 1 applications/argonnite -k 43 -threads-per-node 4 ~/dropbox/mini.fastq
