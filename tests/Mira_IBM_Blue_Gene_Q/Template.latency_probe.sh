#!/bin/bash

# echo "Commit= __COMMIT__"

qsub \
 --env PAMID_THREAD_MULTIPLE=1 \
 -A CompBIO \
 -n 1024 \
 -t __WALLTIME__ \
 -O __JOB__ \
 --mode c1 \
     __JOB__.__APP__ -print-thorium-data -threads-per-node 16
