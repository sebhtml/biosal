#!/bin/bash

./scripts/lttng/build.sh

time (

rm -rf output

# create a session with an automatic name
lttng create

# enable all userspace tracepoints
lttng enable-event --userspace message:* --filter "message_source_actor == 1001563"

lttng start

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 7 ~/dropbox/S.aureus.fasta.gz

lttng stop

# destroy current session
lttng destroy
) | tee log
