#!/bin/bash

echo -n "Tag names :"
grep 0x0000 * -R|grep -v BSAL_HASH|grep -v check-tags.sh|awk '{print $2}'|sort|uniq -c|awk '{print $1}'|sort -n|uniq -c

echo -n "Tag values :"
grep 0x0000 * -R|grep -v BSAL_HASH|grep -v check-tags.sh|awk '{print $3}'|sort|uniq -c|awk '{print $1}'|sort -n|uniq -c
grep 0x0000 * -R|grep -v BSAL_HASH|grep -v check-tags.sh|awk '{print $3}'|sort|uniq -c|grep -v " 1 "

min=0
max=32767

exit

# We don't need to verify the minimum and maximum now.
for tag in $(grep 0x * -R|grep -v ')'|grep -v BSAL_HASH|grep -v table | grep -v SCRIPT|grep -v check-tags.sh|awk '{print $3}'|grep 0x)
do
    #echo $tag
    if [[ $tag -lt $min ]]
    then
        echo "Error $tag < $min"
    fi

    if [[ $tag -gt $max ]]
    then
        echo "Error $tag > $max"

        grep $tag * -R
    fi
done
