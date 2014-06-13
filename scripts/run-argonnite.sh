#!/bin/bash

mpiexec -n 4 ./argonnite -threads-per-node 7 ~/dropbox/*fastq
