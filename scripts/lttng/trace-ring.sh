#!/bin/bash

./scripts/lttng/build.sh

time (

rm -rf output

# create a session with an automatic name
lttng create

# enable all userspace tracepoints
lttng enable-event --userspace ring:*

lttng start

# 3 threads: 1 pacing thread (1 consumer thread) and 2 worker threads (2 producer threads)
mpiexec -n 1 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 3 ~/dropbox/S.aureus.fasta.gz

lttng stop

lttng view > trace.txt

# destroy current session
lttng destroy

echo ""
echo ""
echo "===================> see trace.txt <==================="
echo ""
echo ""
) | tee log
