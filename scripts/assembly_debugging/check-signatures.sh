#!/bin/bash

file=$1

# >path_4990095754155914678 length=5708 circular=0 signature=496a062284e721fa
grep '>' $file > names

cat names|awk '{print $1}' | sed 's/>//g' | sort | uniq > uniq-names

for i in $(cat uniq-names)
do
    echo -n $i
    echo -n " "
    grep $i names | sort|uniq| wc -l
done | grep -v " 1"$
