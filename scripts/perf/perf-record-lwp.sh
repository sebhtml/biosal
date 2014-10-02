#!/bin/bash

# Do get thread identifiers:
# ps -eLf | grep app-name
thread=$1

perf record -a -g -s -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations -t $thread -o $thread.perf.data
