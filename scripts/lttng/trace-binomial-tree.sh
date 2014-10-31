#!/bin/bash

lttng create
lttng enable-event -u thorium_actor:receive_enter
lttng enable-event -u thorium_binomial_tree:*
lttng start
time ./scripts/assembly_debugging/spate-saureus-fasta-gz-4x8-load.sh |tee log

# wait
# do CTRL-C
lttng stop
lttng view > trace
lttng destroy
