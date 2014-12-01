#!/bin/bash

mpiexec -n 4 applications/argonnite_kmer_counter/argonnite -print-thorium-data -k 43 -threads-per-node 8 ~/dropbox/Iowa_Continuous_Corn/*
