#!/bin/bash

mpiexec -n 4 performance/latency_probe/latency_probe -threads-per-node 8 \
    -print-thorium-data \
    -ping-event-count-per-actor 200000 \
