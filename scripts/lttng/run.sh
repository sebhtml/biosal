#!/bin/bash

./scripts/lttng/build.sh

time (

rm -rf output

# create a session with an automatic name
lttng create

# enable all userspace tracepoints
lttng enable-event --userspace --all

lttng start

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 8 ~/dropbox/S.aureus.fasta.gz

lttng stop

# destroy current session
lttng destroy
) | tee log
