#!/bin/bash

# echo "Commit= __COMMIT__"

# -print-thorium-data

qsub \
 --env PAMID_THREAD_MULTIPLE=1 \
 -A CompBIO \
 -n 1024 \
 -t 01:00:00 \
 -O __JOB__ \
 --mode c1 \
     __JOB__.__APP__ -threads-per-node 16 \
    -k 27 __SAMPLE__/*.fastq -print-thorium-data \
    -o __JOB__
