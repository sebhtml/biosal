#!/bin/bash

mpiexec -n 4 applications/argonnite -k 43 -threads-per-node 7 ~/dropbox/*fastq
