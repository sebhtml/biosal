#!/bin/bash

echo "check loop-log"
while true
do
    (
    time ./scripts/development/medium-4-8.sh
    )
done &> loop-log
