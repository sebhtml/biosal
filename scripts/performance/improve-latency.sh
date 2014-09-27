#!/bin/bash

mpiexec -n 4 ./performance/fairness_checker/fairness_checker -threads-per-node 8 \
     -enable-actor-load-profiler
