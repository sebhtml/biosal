#!/bin/bash

# define this in the code: BSAL_MEMORY_DEBUG_DETAIL

grep bsal_memory_free log|awk '{print $3}' > free-addr
grep bsal_memory_allocate log|awk '{print $5}' > allocate-addr
cat allocate-addr  free-addr |sort|uniq -c|sort -n|grep " 1 " | awk '{print $2}' > leaks.txt
for i in $(cat leaks.txt ); do grep $i log|grep _allocate; done > 1
mv 1 leaks.txt

echo "see leaks.txt"
