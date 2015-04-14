#!/bin/bash

cores=$(cat /proc/cpuinfo|grep "model name"|wc -l)

cores=$(($cores / 2))

if test $cores -lt 1
then
    cores=1
fi

cores=2

make -j applications/spate_metagenome_assembler/spate

if ! test -d velvet
then
    git clone https://github.com/dzerbino/velvet.git
fi

if test -d spate_output
then
    rm -rf spate_output
fi

mpiexec -n 1 applications/spate_metagenome_assembler/spate -k 25 -threads-per-node $cores \
    velvet/data/test_reads.fa \
    -o spate_output \
    -enable-actor-log "biosal_tip_manager" \
   -enable-actor-log all 2>&1 | tee Log \

exit

