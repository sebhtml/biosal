#!/bin/bash

time mpiexec -n 4 --map-by node -machinefile /cluster/machines.txt \
    ./latency_probe -threads-per-node 4 -print-thorium-data \
    -ping-event-count-per-actor 20000 | tee log

