#!/usr/bin/env python

# MPI_TAG_UB >= 32767
import random

value = random.randint(0, 32767)
hexadecimal = hex(value).replace('x', '')

while len(hexadecimal) != 8:
    hexadecimal = '0' + hexadecimal

print('0x' + hexadecimal)
