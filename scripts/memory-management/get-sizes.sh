#!/bin/bash

grep core_memory_allocate log|awk '{print $3}' |sort|uniq -c|sort -r -n|head
