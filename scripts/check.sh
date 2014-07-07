#!/bin/bash

function run_check() {
    pattern=$1
    grep -n "$pattern"  * -R|grep -v git|grep -v check.sh
}

function run_checks() {
    run_check "struct  "
    run_check "enum  "
    run_check "enum{"

    run_check "for("
    run_check "if("
    run_check "switch("
    run_check "while("

    run_check "for  ("
    run_check "if  ("
    run_check "switch  ("
    run_check "while  ("

    run_check "  ="
    run_check "=  "
    run_check ")  {"
    run_check "}  ("

    grep -n ") {" * -R |grep void|grep bsal_|grep -v check.sh|grep -v if|grep -v while
    grep -n " "$ * -R
    grep -n " -> " * -R | grep -v printf|grep -v ^log
}

make clean
clear

echo "=== Quality Assurance ($(date)) ==="

echo "User: $(whoami)"
echo "Host: $(hostname)"
echo ""

git branch
echo ""
git diff --stat energy
echo ""
git status
echo ""

echo ""

echo "Detected issues: $(run_checks|wc -l)"

run_checks

./scripts/check-tags.sh

./scripts/check-scripts.sh

echo "Thank you."
