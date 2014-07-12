#!/bin/bash

function run_example()
{
    local example

    example=$1

    echo -n "Running test case example:$example ..."

    (
    time make $example
    ) &> $example.log

    echo " see $example.log"
}

function main()
{
    local example

    for example in mock mock1 ring reader not_found remote_spawn synchronize controller hello_world clone migration
    do
        run_example $example
    done
}

main
