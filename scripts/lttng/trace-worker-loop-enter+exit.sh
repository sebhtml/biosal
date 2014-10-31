#!/bin/bash

duration=1

lttng create
lttng enable-event \
    -u thorium_worker:run_enter,thorium_worker:run_exit \
    --filter "worker == 3"
echo "Tracing for $duration seconds"
lttng start
sleep $duration
lttng stop
lttng view > worker.trace
wc -l worker.trace
lttng destroy

