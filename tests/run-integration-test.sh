#!/bin/bash

function main()
{
    local test_case

    test_case=$1

    (
    time scripts/development/$test_case.sh
    ) &> log
    cat log
}

main $1
