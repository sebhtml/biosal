#!/bin/bash

echo "This does not include Thorium action specifiers, which are negative."

echo "Tag names :"
grep " ACTION_" * -R|grep define|grep -v check-tags|grep -v _BASE|awk '{print $2}'|sort | uniq -c | grep " 1 " | wc -l
grep " ACTION_" * -R|grep define|grep -v check-tags|grep -v _BASE|awk '{print $2}'|sort | uniq -c | grep -v " 1 "

echo "Tag values :"
grep " ACTION_" * -R|grep define|grep -v check-tags|grep -v _BASE|awk '{print $3}'|sort | uniq -c | grep " 1 " | wc -l
grep " ACTION_" * -R|grep define|grep -v check-tags|grep -v _BASE|awk '{print $3}'|sort | uniq -c | grep -v " 1 "

