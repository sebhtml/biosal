#!/bin/bash

echo "Actor scripts: "
grep _SCRIPT * -R|grep define|grep -v check-scripts.sh|awk '{print $2" "$3}'|wc -l
echo ""

echo "Actor script name uniqueness:"
grep _SCRIPT * -R|grep define|grep -v check-scripts.sh|awk '{print $2}'|sort | uniq -c|awk '{print $1}'|sort -n|uniq -c
echo ""

echo "Actor script value uniqueness:"
grep _SCRIPT * -R|grep define|grep -v check-scripts.sh|awk '{print $3}'|sort | uniq -c|awk '{print $1}'|sort -n|uniq -c
echo ""

echo "Actor scripts: "
grep _SCRIPT * -R|grep define|grep -v check-scripts.sh|awk '{print $2" "$3}'
echo ""

