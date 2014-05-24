#!/bin/bash

make clean
grep -n "if  ("  * -R|grep -v git
grep -n "if("  * -R|grep -v git
grep -n "for(" * -R|grep -v git
grep -n "for  (" * -R|grep -v git
grep -n "  =" * -R|grep -v git
grep -n "=  " * -R|grep -v git
grep -n " $" * -R|grep -v git
grep -n "	" * -R|grep -v Makefile|grep -v git
