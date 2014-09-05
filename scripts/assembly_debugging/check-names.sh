#!/bin/bash

file=$1

grep '>' $file > raw-names
cat raw-names|awk '{print $1}'|sort|uniq -c |awk '{print $2}'> names

echo "Got names"

for i in $(cat names)
do
    grep $i raw-names
done|sort | uniq -c |awk '{print $2}' | sort|uniq -c|grep -v " 1 " | awk '{print $2}' > defect-names

echo "Got defect names"

for i in $(cat defect-names)
do
    echo "Defect: $i"
    grep $i raw-names
    echo ""
done > DefectList.txt

echo "Check DefectList.txt (defects: $(cat DefectList.txt |grep ^Defect| wc -l) / $(cat raw-names | wc -l))"
