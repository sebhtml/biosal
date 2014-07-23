#!/usr/bin/env python

import sys
import os

def print_context(frame, assembly_file, address):
    address_without_0x = address.replace("0x", "").replace("[", "").replace("]", "")

    function = "NULL"
    instruction = "NULL"

    for line in open(assembly_file):
        if len(line) >= 4 and line[0] == "0" and line[1] == "0" and line[2] == "0":
            function = line.strip().split()[1].replace(":", "").replace("<", "")
        elif line.find(address_without_0x + ":") >= 0:
            instruction = line.strip()
            break

    print("#" + str(frame) + " " + address + " " + function + " " + instruction)

def main(argc, argv):
    #print(argv)

    if argc != 3:
        print("Usage: disassemble-and-get-stack.py -e executable < stack")
        sys.exit()


    executable = argv[2]

    #print(executable)

    assembly_file = executable + ".s"

    os.system("objdump -d " + executable + " > " + assembly_file)

    frame = 0
    for line in sys.stdin:
        tokens = line.split()

        address = tokens[0]


        if len(tokens) > 1:
            address = tokens[1]

        print_context(frame, assembly_file, address)

        frame += 1


if __name__ == "__main__":
    main(len(sys.argv), sys.argv)


