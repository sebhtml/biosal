#!/bin/bash

mpiexec -n 2 performance/latency_probe/latency_probe -threads-per-node 3 -print-load
