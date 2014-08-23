#!/usr/bin/env python

# MPI_TAG_UB >= 32767
# Update: the tag is no longer stored using the tag argument.
# It is stored in the buffer.

import random

# max = 32767
# the maximum value of a int is 2147483647
max = 4194304

value = random.randint(0, max)
hexadecimal = hex(value).replace('x', '')

#print("DEBUG " + hexadecimal)

while len(hexadecimal) != 8:
    hexadecimal = '0' + hexadecimal

print('#define ACT_CHANGE_ME 0x' + hexadecimal)
