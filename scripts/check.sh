#!/bin/bash

make clean
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
