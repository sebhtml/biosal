#!/bin/bash

grep core_memory_allocate log|awk '{print $10}' |sort|uniq -c|sort -r -n|head
