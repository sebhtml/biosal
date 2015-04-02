#!/bin/bash

./scripts/build/build.sh

duration=9999

timeout $duration \
    mpiexec -n 8 applications/spate_metagenome_assembler/spate -k 43 -threads-per-node 1 ~/dropbox/mini.fastq \
    -enable-actor-log all 2>&1 | tee Log

grep XXX Log
