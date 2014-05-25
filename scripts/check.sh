#!/bin/bash

function run_checks() {
    grep -n "struct  "  * -R|grep -v git
    grep -n "enum  "  * -R|grep -v git
    grep -n "if  ("  * -R|grep -v git
    grep -n "if("  * -R|grep -v git
    grep -n "for(" * -R|grep -v git
    grep -n "for  (" * -R|grep -v git
    grep -n " -> " * -R|grep -v git | grep -v printf
    grep -n "  =" * -R|grep -v git
    grep -n "=  " * -R|grep -v git
    grep -n " $" * -R|grep -v git
    grep -n "	" * -R|grep -v Makefile|grep -v git
}

clear

echo "=== Quality Assurance ($(date)) ==="

echo "User: $(whoami)"
echo "Host: $(hostname)"

git branch
echo ""
git diff --stat energy
echo ""
git status
echo ""

make clean

echo ""

echo "Detected issues: $(run_checks|wc -l)"

run_checks

echo "Thank you."
