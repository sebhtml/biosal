#!/bin/bash

(find .|grep '\.c'$;find .|grep '\.h'$) > files
count=$(cat files | wc -l)
random_number=$RANDOM
line=$(($random_number % $count))

file_name=$(head -n $line  files | tail -n 1)

vim $file_name
