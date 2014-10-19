#!/bin/bash

./scripts/lttng/build.sh

time (

rm -rf output

# create a session with an automatic name
lttng create

# enable all userspace tracepoints
lttng enable-event --userspace thorium_message:* --filter "message_source_actor == 1001563"

lttng start

mpiexec -n 4 ./applications/spate_metagenome_assembler/spate -k 51 -threads-per-node 7 ~/dropbox/S.aureus.fasta.gz

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

cat trace.txt | grep "message_number = 99,"|awk '{print $4" "$2}'


