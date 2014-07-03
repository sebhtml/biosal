#!/bin/bash

for i in $(find core|grep .c$; find genomics|grep .c$)
do
    gcov-4.7 $i
done &> /dev/null

cat $(ls *gcov | grep hot_code)|grep -v '\-'|grep -v '#'|sed 's/://g'|sort -r -n > hot_code_temperature.txt
cat $(ls *gcov | grep -v hot_code)|grep -v '\-'|grep -v '#'|sed 's/://g'|sort -r -n > code_temperature.txt

cat $(ls *gcov | grep hot_code)|grep -v '\-'|grep -v '#'|sed 's/://g'|sort -r -n | grep bsal_|grep -v ';'|grep -v ' if ' > hot_code_temperature_functions.txt
cat $(ls *gcov | grep -v hot_code)|grep -v '\-'|grep -v '#'|sed 's/://g'|sort -r -n  | grep bsal_|grep -v ';'|grep -v ' if '> code_temperature_functions.txt

echo "Check code_temperature.txt and hot_code_temperature.txt"
echo "Check code_temperature_functions.txt and hot_code_temperature_functions.txt"
