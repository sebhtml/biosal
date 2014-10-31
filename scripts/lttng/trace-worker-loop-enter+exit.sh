#!/bin/bash

# the node and worker must be provided in arguments

node=$1
worker=$2

duration=1

lttng create
lttng enable-event \
    -u thorium_worker:run_enter,thorium_worker:run_exit \
    --filter "node == $node && worker == $worker"
echo "Tracing for $duration seconds"
lttng start
sleep $duration
lttng stop
lttng view > worker-$node-$worker.trace
lttng destroy

