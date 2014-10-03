#!/bin/bash

# Do get thread identifiers:
# ps -eLf | grep app-name
thread=$1

# -a System-wide collection from all CPUs.
# -g Enables call-graph (stack chain/backtrace) recording.
# -s Per thread counts.
# -e Select the PMU events

perf top -g -e cache-references,cache-misses,cycles,instructions,branches,faults -t $thread
