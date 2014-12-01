#!/bin/bash

mpiexec -n 4 applications/spate_metagenome_assembler/spate -threads-per-node 8 \
    -k 49 ~/dropbox/SAMN02402663-Mtuberculosis/* -o SAMN02402663-Mtuberculosis-1 -print-thorium-data
