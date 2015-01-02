#!/bin/bash

module swap PrgEnv-cray PrgEnv-gnu

make clean

make CC=cc -j 4 \
        performance/latency_probe/latency_probe \
        applications/spate_metagenome_assembler/spate \

