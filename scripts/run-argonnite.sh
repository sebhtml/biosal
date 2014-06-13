#!/bin/bash

mpiexec -n 4 applications/argonnite -threads-per-node 7 ~/dropbox/*fastq
