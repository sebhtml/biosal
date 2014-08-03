#!/bin/bash

#valgrind --tool=memcheck --leak-check=yes -v \
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --undef-value-errors=no \
        applications/argonnite_kmer_counter/argonnite -k 43 -threads-per-node 1 ~/dropbox/mini.fastq &> valgrind.log
