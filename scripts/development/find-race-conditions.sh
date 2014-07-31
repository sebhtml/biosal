#!/bin/bash

echo "check loop-log"
while true
do
    (
    time make mock1
    )
done &> loop-log
