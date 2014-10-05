#!/bin/bash

# This script will inspect events reported by
# core/system/memory.c with CORE_MEMORY_DEBUG enabled.

level=$1

grep ^"#$level" log|sort|uniq -c|sort -r -n|head
