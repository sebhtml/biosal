#!/bin/bash

make clean

make CC=cc -j 4  \
        applications/spate_metagenome_assembler/spate \
	performance/latency_probe/latency_probe \
