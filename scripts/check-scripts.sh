#!/bin/bash

echo "Script names :"
grep " SCRIPT_" * -R|grep -v "doc/"|grep define|grep -v check-scripts|awk '{print $2}'|sort | uniq -c | grep " 1 " | wc -l
grep " SCRIPT_" * -R|grep -v "doc/"|grep define|grep -v check-scripts|awk '{print $2}'|sort | uniq -c | grep -v " 1 "

echo "Script values :"
grep " SCRIPT_" * -R|grep -v "doc/"|grep define|grep -v check-scripts|awk '{print $3}'|sort | uniq -c | grep " 1 " | wc -l
grep " SCRIPT_" * -R|grep -v "doc/"|grep define|grep -v check-scripts|awk '{print $3}'|sort | uniq -c | grep -v " 1 "

