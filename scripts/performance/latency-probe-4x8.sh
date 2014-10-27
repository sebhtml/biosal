#!/bin/bash

mpiexec -n 4 performance/latency_probe/latency_probe -threads-per-node 8 \
    -print-load
