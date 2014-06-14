#!/bin/bash

mpiexec -n 4 applications/argonnite -k 43 -threads-per-node 8 ~/dropbox/*fastq
