#!/bin/bash

./scripts/lttng/build.sh

time (
rm -rf output

lttng create

lttng enable-event -u \
    thorium_actor:receive_enter,thorium_actor:receive_exit,thorium_actor:send \
    --filter "name == 1000222"

lttng start

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 33 -threads-per-node 7 ~/dropbox/S.aureus.fasta.gz

lttng stop

lttng view > trace.txt

clear

# destroy current session
lttng destroy
) | tee log

echo ""
echo ""

echo "==================> trace.txt <==================== ;-)"

echo ""
echo ""



