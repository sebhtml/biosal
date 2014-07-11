#!/bin/bash

#valgrind --tool=memcheck --leak-check=yes -v \
valgrind --leak-check=full \
        applications/argonnite -k 43 -threads-per-node 32 ~/dropbox/medium-2.fastq
