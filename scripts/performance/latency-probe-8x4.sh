#!/bin/bash

mpiexec -n 8 performance/latency_probe/latency_probe -threads-per-node 4 \
    -print-load
