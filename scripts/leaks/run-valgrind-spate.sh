#!/bin/bash

#valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --undef-value-errors=no \
valgrind --tool=memcheck --leak-check=yes  \
        applications/spate_metagenome_assembler/spate -k 43 -threads-per-node 1 ~/dropbox/mini.fastq &> valgrind.log
