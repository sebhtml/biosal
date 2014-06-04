#!/bin/bash

grep 0x0000 * -R|grep -v check-tags.sh|awk '{print $3}'|sort|uniq -c|awk '{print $1}'|sort -n|uniq -c
