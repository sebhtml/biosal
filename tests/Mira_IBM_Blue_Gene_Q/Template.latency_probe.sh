#!/bin/bash

# echo "Commit= __COMMIT__"

qsub \
 --env PAMID_THREAD_MULTIPLE=1 \
 -A CompBIO \
 -n __NODES__ \
 -t __WALLTIME__ \
 -O __JOB__ \
 --mode c1 \
     __JOB__.__APP__ -print-thorium-data -threads-per-node __THREADS__
