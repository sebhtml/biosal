#!/bin/bash

# echo "Commit= __COMMIT__"

# -print-thorium-data

qsub \
 --env PAMID_THREAD_MULTIPLE=1 \
 -A CompBIO \
 -n __NODES__ \
 -t __WALLTIME__ \
 -O __JOB__ \
 --mode c1 \
     __JOB__.__APP__ -threads-per-node __THREADS__ \
    -k 27 __SAMPLE__/*.fastq \
    -minimum-coverage 2 \
    -print-thorium-data \
    -o __JOB__ \
