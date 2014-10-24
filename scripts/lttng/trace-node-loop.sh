#!/bin/bash

duration=5

lttng create
lttng enable-event -u thorium_node:* --filter " node == 1"
echo "Tracing for $duration seconds"
lttng start
sleep $duration
lttng stop
lttng view > node.trace
wc -l node.trace
lttng destroy

