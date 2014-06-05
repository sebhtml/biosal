#!/bin/bash

echo "Names"
grep 0x0000 * -R|grep -v check-tags.sh|awk '{print $2}'|sort|uniq -c|awk '{print $1}'|sort -n|uniq -c

echo "Values"
grep 0x0000 * -R|grep -v check-tags.sh|awk '{print $3}'|sort|uniq -c|awk '{print $1}'|sort -n|uniq -c
grep 0x0000 * -R|grep -v check-tags.sh|awk '{print $3}'|sort|uniq -c|grep -v " 1 "
