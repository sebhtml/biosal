#!/bin/bash

make clean

make CC=cc -j 4  \
        applications/spate_metagenome_assembler/spate
