#!/bin/bash

(grep death log ; grep node_spawn log)|awk '{print $4}'|sort | uniq -c | sort -n|grep " 1 "|awk '{print $2}'
