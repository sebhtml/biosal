#!/bin/bash

duration=1

lttng create
lttng enable-event -u thorium_node:*
echo "Tracing for $duration seconds"
lttng start
sleep $duration
lttng stop
lttng view > node.trace
wc -l node.trace
lttng destroy

