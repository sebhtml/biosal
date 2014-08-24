#!/bin/bash

if test $# -lt 1
then
    echo "Need a file (gz file)"
    exit
fi

file=$1

mkdir $file"_PROFILES"
mkdir $file"_PROFILES/memory/"
mkdir $file"_PROFILES/load/"
cd $file"_PROFILES"

ln -s ../$file

nodes=$(cat $file | gunzip | grep booted | head -n1|awk '{print $5}'|sed 's=(==g')

echo "Nodes= $nodes"
seq 0 $((nodes - 1)) > NodeList.txt

if test $# -eq 2
then
    echo $2 > NodeList.txt
fi

for node in $(cat NodeList.txt)
do
    echo $node

    cat $file | gunzip | grep ByteCount | grep "node/$node " | awk '{print $9}' > memory/Node.$node".memory"
    cat $file | gunzip | grep LOAD | grep EPOCH | grep "node/$node " | awk '{print $5" " $7}'| sed 's=/= =g'|awk '{print $1" "$2}' > load/Node.$node".load"

    echo "r=read.table('load/Node."$node".load')
    png('load/Node."$node".load.png')
    plot(r[,1], r[,2], type='l', col='blue', xlab='Time (seconds)', ylab='Load', main='Load for node $node')
    dev.off()" | R --vanilla --slave --quiet

    echo "r=read.table('memory/Node."$node".memory')
    png('memory/Node."$node".memory.png')
    plot(1:length(r[,1]), r[,1], type='l', col='blue', xlab='Time', ylab='Memory', main='Memory for node $node')
    dev.off()" | R --vanilla --slave --quiet

done

