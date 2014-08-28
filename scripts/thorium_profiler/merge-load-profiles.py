#!/usr/bin/env python

import sys

if len(sys.argv) != 2:
    sys.exit()

count = int(sys.argv[1])

i = 0

while i < count:
    file = "Node." + str(i) + ".load"
    for line in open(file):
        print(str(i) + " " + line.strip())
    print("")
    i += 1
