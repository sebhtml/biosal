#!/bin/bash

# How to use:
#
# define BSAL_MEMORY_DEBUG in core/system/memory.h (#define BSAL_MEMORY_DEBUG)
# Run a job and put the standard output in a file (like 'log')
#
# Then run:
#   scripts/leaks/manual-search.sh log

grep bsal_memory_allocate log|grep DEBUG |awk '{print $5}' > allocate-addr
grep bsal_memory_free log|grep DEBUG |awk '{print $3}' > free-addr
cat allocate-addr  free-addr |sort|uniq -c|sort -n|grep " 1 " | awk '{print $2}' > leaks.txt

echo "see leaks.txt"
