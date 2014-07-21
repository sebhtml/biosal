#!/bin/bash

#echo "Actor scripts: "
#grep _SCRIPT * -R|grep define|grep -v check-scripts.sh|awk '{print $2" "$3}'|wc -l
#echo ""

echo -n "Script names: "
grep "_SCRIPT " * -R|grep define|grep -v check-scripts.sh|awk '{print $2}'|sort | uniq -c|awk '{print $1}'|sort -n|uniq -c

echo -n "Script values: "
grep "_SCRIPT " * -R|grep define|grep -v check-scripts.sh|awk '{print $3}'|sort | uniq -c|awk '{print $1}'|sort -n|uniq -c

#echo "Actor scripts: "
#grep _SCRIPT * -R|grep define|grep -v check-scripts.sh|awk '{print $2" "$3}'
echo ""

