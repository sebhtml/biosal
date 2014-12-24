#!/bin/bash


mpiexec -n 4 performance/latency_probe/latency_probe -threads-per-node 8 \
    -ping-event-count-per-actor 10000 \
    -print-thorium-data \
    -degree-of-aggregation-limit 20 \
    -aggregation-timeout 100000 \
    -enable-actor-log all \
