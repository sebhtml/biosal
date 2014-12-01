#!/bin/bash

qsub \
 --env PAMID_THREAD_MULTIPLE=1 \
 -A CompBIO \
 -n 2048 \
 -t 00:30:00 \
 -O strong-scaling-569-spate-thorium-cetus-2048x16-200 \
 --mode c1 \
 spate -print-thorium-data -threads-per-node 16 -k 43 Iowa_Continuous_Corn/*.fastq -o strong-scaling-569-spate-thorium-cetus-2048x16-200
