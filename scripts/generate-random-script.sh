#!/bin/bash

echo -n "#define SCRIPT_CHANGE_ME 0x"
dd if=/dev/random count=1 bs=30 2> /dev/null |sha1sum|awk '{print substr($0,0,8)}'
