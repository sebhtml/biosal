#!/usr/bin/env python

import sys

threshold = 300

enter = 0
exit = 0

for line in sys.stdin:
    tokens = line.split()
    time_tokens = tokens[0].split(":")
    time = float(time_tokens[len(time_tokens) - 1].replace("]", ""))
    event = tokens[3].split(":")[1]

    print(line.strip())
    if event == "tick_enter":
        enter = time
    elif event == "tick_exit":
        exit = time

        elapsed = (exit - enter) * 1000 * 1000

        if elapsed >= threshold:
            print("ELAPSED= " + str(elapsed))
            print("")

    #print(event + " " + str(time))
