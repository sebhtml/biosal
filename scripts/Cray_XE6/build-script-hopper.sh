#!/bin/bash

module swap PrgEnv-pgi PrgEnv-gnu
make clean
make CC=cc -j 4 \
	applications/spate_metagenome_assembler/spate


