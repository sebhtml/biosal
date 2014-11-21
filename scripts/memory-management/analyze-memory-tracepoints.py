#!/usr/bin/env python

#
# usage ./analyze-memory-tracepoints.py < spate-2014-11-19-02-24-12-3.01020.txt

import sys
import os

# allocation:
# TRACEPOINT memory:allocate size= 24 pointer= 0x19c64b0060 function= core_memory_pool_add_block file= core/system/memory_pool.c line= 421 key= 0x97b478ba

# free:
# TRACEPOINT memory:free pointer= 0x19d7750000 function= thorium_worker_pool_print_load file= engine/thorium/worker_pool.c line= 458 key= 0x533932bf

systems = {}

pointers = {}

event_count = 0

for line in sys.stdin:
    tokens = line.split()
    if len(tokens) == 0:
        continue
    type = tokens[0]
    if type != "TRACEPOINT":
        continue

    event = tokens[1]

    event_count += 1

    if event == "memory:allocate":
        pointer = tokens[5]
        size = int(tokens[3])
        key = tokens[13]

        if key not in systems:
            systems[key] = {}
            systems[key]["memory_usage"] = 0
        systems[key]["memory_usage"] += size
        pointers[pointer] = size

    elif event == "memory:free":
        pointer = tokens[3]
        key = tokens[11]
        size = pointers[pointer]

        systems[key]["memory_usage"] -= size
        del pointers[pointer]

print("Memory usage after " + str(event_count) + " events")

total = 0

keys = systems.keys()
keys.sort()

def get_key_name(key):

    if len(key) < 2 + 8:
        key = key.replace("0x", "0x0")

    command = "grep " + key + " $HOME/biosal/* -R|grep -v DEBUG_KEY|head -n1|awk '{print $2}'"

    #print("Command= " + command)

    name = os.popen(command).read().strip()

    if name == "":
        name = key

    return name

print("")

print("| Key | memory:allocate event count | memory:free event count | Byte count |")
print("| --- | --- | --- | --- |")

for key in keys:
    value = systems[key]["memory_usage"]
    print("| " + get_key_name(key) + " | | | " + str(value) + " |")
    total += value

print("| Total | | | " + str(total) + " |")
