#!/bin/bash

duration=5

lttng create
lttng enable-event \
    -u thorium_worker:publish_message,thorium_worker:flush_outbound_message_block \
    --filter "worker == 3"
echo "Tracing for $duration seconds"
lttng start
sleep $duration
lttng stop
lttng view > trace
wc -l trace
lttng destroy

