#!/bin/bash

perf record -a -g -s -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations -- \
	applications/argonnite_kmer_counter/argonnite -print-thorium-data -k 43 -threads-per-node 4 ~/dropbox/mini.fastq
