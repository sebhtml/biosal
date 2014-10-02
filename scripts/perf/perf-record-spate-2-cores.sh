#!/bin/bash

perf record -a -g -s -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations -- \
        applications/spate_metagenome_assembler/spate -k 43 -threads-per-node 2 ~/dropbox/S.aureus.fasta.gz
