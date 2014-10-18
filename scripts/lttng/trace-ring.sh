#!/bin/bash

./scripts/lttng/build.sh

time (

rm -rf output

# create a session with an automatic name
lttng create

# enable all userspace tracepoints
lttng enable-event --userspace ring:*

lttng start

mpiexec -n 1 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 30 ~/dropbox/S.aureus.fasta.gz

lttng stop

# destroy current session
lttng destroy
) | tee log
