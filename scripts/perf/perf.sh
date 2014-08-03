#!/bin/bash

perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations \
	applications/argonnite_kmer_counter/argonnite -print-load -k 43 -threads-per-node 4 ~/dropbox/mini.fastq
